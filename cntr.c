#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <locale.h>
#include <wchar.h>
#include <ctype.h>
#include <errno.h> // For errno and perror
#include <wctype.h>

/**
 * Gets the current width of the terminal
 *
 * @return Width of the terminal in characters or a default value on error
 */
int getTerminalWidth()
{
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1)
    {
        return 80; // Default width if ioctl fails
    }
    return w.ws_col;
}

/**
 * Calculates the display width of a UTF-8 string
 *
 * @param str UTF-8 encoded string
 * @return Number of visual characters (not bytes)
 */
int getDisplayWidth(const char *str)
{
    int width = 0;
    mbstate_t state;
    memset(&state, 0, sizeof(state));

    const char *ptr = str;
    size_t len = strlen(str); // Get total string length
    while (len > 0)
    { // Use len instead of *ptr != '\0' for safety with mbrtowc
        wchar_t wc;
        int consumed = mbrtowc(&wc, ptr, len, &state);

        if (consumed == (size_t)-1 || consumed == (size_t)-2)
        {
            // Invalid sequence, treat as 1 char wide
            // fprintf(stderr, "Warning: Invalid UTF-8 sequence found.\n");
            width++; // Count it as 1 for safety
            ptr++;
            len--;
            memset(&state, 0, sizeof(state)); // Reset state
            continue;
        }
        else if (consumed == 0)
        {
            // Null character reached
            break;
        }

        int char_width = wcwidth(wc);
        if (char_width < 0)
            char_width = 1; // Treat non-printable/control characters as width 1
        width += char_width;
        ptr += consumed;
        len -= consumed;
    }

    return width;
}

/**
 * Structure for a text paragraph
 */
typedef struct
{
    char **lines;  // Array of lines in the paragraph
    int lineCount; // Number of lines
    int capacity;  // Capacity of the lines array
} Paragraph;

/**
 * Initializes a new paragraph
 */
Paragraph *createParagraph()
{
    Paragraph *para = (Paragraph *)malloc(sizeof(Paragraph));
    if (!para)
    {
        perror("Memory allocation error for Paragraph");
        return NULL;
    }

    para->capacity = 8; // Initial capacity
    para->lineCount = 0;
    para->lines = (char **)malloc(para->capacity * sizeof(char *));

    if (!para->lines)
    {
        perror("Memory allocation error for Paragraph lines");
        free(para);
        return NULL;
    }

    return para;
}

/**
 * Adds a line to a paragraph
 */
void addLineToParagraph(Paragraph *para, const char *line)
{
    if (!para)
        return;
    if (para->lineCount >= para->capacity)
    {
        int newCapacity = para->capacity * 2;
        char **newLines = (char **)realloc(para->lines, newCapacity * sizeof(char *));
        if (!newLines)
        {
            perror("Reallocation error for Paragraph lines");
            // Line cannot be added
            return;
        }
        para->lines = newLines;
        para->capacity = newCapacity;
    }

    para->lines[para->lineCount] = strdup(line);
    if (!para->lines[para->lineCount])
    {
        perror("strdup error for Paragraph line");
        return;
    }
    para->lineCount++;
}

/**
 * Frees the memory of a paragraph
 */
void freeParagraph(Paragraph *para)
{
    if (!para)
        return;
    for (int i = 0; i < para->lineCount; i++)
    {
        free(para->lines[i]);
    }
    free(para->lines);
    free(para);
}

/**
 * Structure for the entire document
 */
typedef struct
{
    Paragraph **paragraphs; // Array of paragraphs
    int paragraphCount;     // Number of paragraphs
    int capacity;           // Capacity of the paragraphs array
} Document;

/**
 * Initializes a new document
 */
Document *createDocument()
{
    Document *doc = (Document *)malloc(sizeof(Document));
    if (!doc)
    {
        perror("Memory allocation error for Document");
        return NULL;
    }

    doc->capacity = 8; // Initial capacity
    doc->paragraphCount = 0;
    doc->paragraphs = (Paragraph **)malloc(doc->capacity * sizeof(Paragraph *));

    if (!doc->paragraphs)
    {
        perror("Memory allocation error for Document paragraphs");
        free(doc);
        return NULL;
    }

    return doc;
}

/**
 * Adds a paragraph to a document
 */
void addParagraphToDocument(Document *doc, Paragraph *para)
{
    if (!doc || !para)
        return;
    if (doc->paragraphCount >= doc->capacity)
    {
        int newCapacity = doc->capacity * 2;
        Paragraph **newParagraphs = (Paragraph **)realloc(doc->paragraphs, newCapacity * sizeof(Paragraph *));
        if (!newParagraphs)
        {
            perror("Reallocation error for Document paragraphs");
            // Paragraph cannot be added
            return;
        }
        doc->paragraphs = newParagraphs;
        doc->capacity = newCapacity;
    }

    doc->paragraphs[doc->paragraphCount] = para;
    doc->paragraphCount++;
}

/**
 * Frees the memory of a document
 */
void freeDocument(Document *doc)
{
    if (!doc)
        return;
    for (int i = 0; i < doc->paragraphCount; i++)
    {
        freeParagraph(doc->paragraphs[i]);
    }
    free(doc->paragraphs);
    free(doc);
}

/**
 * Splits a string at word boundaries to stay within the given maximum width.
 * Considers the actual display width of UTF-8 characters.
 * (This function is not currently used directly, as parseDocument works line by line)
 *
 * @param text The text to split
 * @param maxWidth The maximum width of a line
 * @return A newly allocated Paragraph with the split lines
 */
Paragraph *wrapTextToWidth(const char *text, int maxWidth)
{
    Paragraph *para = createParagraph();
    if (!para)
        return NULL;

    // Temporary buffer for the current line
    // Size: Length of the entire paragraph + 1 should be sufficient
    size_t textLen = strlen(text);
    char *currentLine = (char *)malloc(textLen + 1);
    if (!currentLine)
    {
        perror("Memory allocation failed for currentLine in wrapTextToWidth");
        freeParagraph(para);
        return NULL;
    }
    currentLine[0] = '\0';
    // Word buffer, also generously sized
    char *word = (char *)malloc(textLen + 1);
    if (!word)
    {
        perror("Memory allocation failed for word in wrapTextToWidth");
        free(currentLine);
        freeParagraph(para);
        return NULL;
    }

    int currentLineWidth = 0;
    const char *ptr = text;
    mbstate_t word_state; // For getDisplayWidth inside the loop

    while (*ptr)
    {
        // Skip leading whitespace
        while (*ptr && iswspace(btowc(*ptr)))
        { // Safer with iswspace for Unicode
            ptr++;
        }

        if (!*ptr)
            break; // End of text reached after whitespace

        // Extract the next word (sequence without whitespace)
        int i = 0;
        while (*ptr && !iswspace(btowc(*ptr)))
        {
            // Copy character by character into the word buffer
            // Ensure the buffer does not overflow (should not happen with textLen+1)
            if (i < textLen)
            {
                word[i++] = *ptr;
            }
            ptr++;
        }
        word[i] = '\0'; // Terminate the word

        memset(&word_state, 0, sizeof(word_state)); // Reset state for getDisplayWidth
        int wordWidth = getDisplayWidth(word);      // Width of the extracted word

        // Check if the word (plus potentially a space) fits on the current line
        if (currentLineWidth == 0 || (currentLineWidth + 1 + wordWidth <= maxWidth))
        {
            // Add a space if the line is not empty
            if (currentLineWidth > 0)
            {
                // Ensure there is space for the space and the word
                if (strlen(currentLine) + 1 < textLen)
                {
                    strcat(currentLine, " ");
                    currentLineWidth += 1; // Width of the space
                }
            }

            // Add the word to the current line
            // Ensure there is space for the word
            if (strlen(currentLine) + strlen(word) < textLen)
            {
                strcat(currentLine, word);
                currentLineWidth += wordWidth;
            }
        }
        else
        {
            // Word doesn't fit anymore: Finalize the current line and add it to the paragraph
            addLineToParagraph(para, currentLine);

            // The current word becomes the start of the new line
            // Ensure the word fits in the buffer (it should)
            if (strlen(word) < textLen + 1)
            {
                strcpy(currentLine, word);
                currentLineWidth = wordWidth;
            }
            else
            {
                // Should not happen, but clear line for safety
                currentLine[0] = '\0';
                currentLineWidth = 0;
            }
        }
    } // End while(*ptr)

    // Add the last line if it has content
    if (currentLineWidth > 0)
    {
        addLineToParagraph(para, currentLine);
    }

    free(word);
    free(currentLine);

    return para;
}

/**
 * Parses a string into a document structure by recognizing paragraphs
 * (separated by double newlines) and preserving existing line breaks within them.
 *
 * @param text The text to parse
 * @return A newly allocated Document structure or NULL on error.
 */
Document *parseDocument(const char *text)
{
    Document *doc = createDocument();
    if (!doc)
        return NULL;

    const char *textPtr = text; // Pointer to the current position in the text

    while (*textPtr)
    {
        // Skip leading empty lines between paragraphs
        while (*textPtr == '\n')
        {
            textPtr++;
        }
        if (!*textPtr)
            break; // End of text

        // Find the start of the next paragraph
        const char *paraStart = textPtr;

        // Find the end of the current paragraph (double newline or end of string)
        const char *paraEnd = strstr(paraStart, "\n\n");
        const char *nextParaStart = NULL;

        if (paraEnd == NULL)
        {
            // No \n\n found, paragraph goes to the end of the string
            paraEnd = paraStart + strlen(paraStart);
            nextParaStart = paraEnd; // At the end of the text
        }
        else
        {
            // \n\n found, marks the end of the paragraph
            nextParaStart = paraEnd + 2; // Skip the two \n
        }

        // Create a new paragraph for the document
        Paragraph *para = createParagraph();
        if (!para)
        {
            freeDocument(doc); // Cleanup
            return NULL;       // Error during paragraph creation
        }

        // Process the current paragraph line by line
        const char *lineStart = paraStart;
        while (lineStart < paraEnd && *lineStart)
        {
            // Find the end of the current line (\n or end of paragraph)
            const char *lineEnd = lineStart;
            while (lineEnd < paraEnd && *lineEnd != '\n')
            {
                lineEnd++;
            }

            // Extract the line
            int lineLength = lineEnd - lineStart;
            char *line = (char *)malloc(lineLength + 1);
            if (!line)
            {
                perror("Memory allocation failed for line in parseDocument");
                freeParagraph(para);
                freeDocument(doc);
                return NULL;
            }
            strncpy(line, lineStart, lineLength);
            line[lineLength] = '\0';

            // Add the extracted line to the paragraph
            // NOTE: The current design adds lines directly.
            // If word wrapping *within* paragraphs based on terminal width is desired,
            // `wrapTextToWidth` (or similar logic) would need to be called here.
            // Currently, the input's structure (including line breaks) is preserved.
            addLineToParagraph(para, line);
            free(line); // strdup was used in addLineToParagraph

            // Go to the start of the next line
            lineStart = lineEnd;
            if (lineStart < paraEnd && *lineStart == '\n')
            {
                lineStart++; // Skip the \n
            }
            // End of inner loop (line processing)
        } // End while (lineStart < paraEnd)

        // Add the filled paragraph to the document
        addParagraphToDocument(doc, para);

        // Go to the start of the next paragraph
        textPtr = nextParaStart;

    } // End while (*textPtr) - main loop over paragraphs

    return doc;
}

/**
 * Prints a document centered on the terminal.
 *
 * @param doc The document to print.
 */
void printCenteredDocument(Document *doc)
{
    if (!doc)
        return;
    const int terminalWidth = getTerminalWidth();

    for (int i = 0; i < doc->paragraphCount; i++)
    {
        Paragraph *para = doc->paragraphs[i];
        if (!para)
            continue;

        for (int j = 0; j < para->lineCount; j++)
        {
            const char *line = para->lines[j];
            if (!line)
                continue;

            int displayWidth = getDisplayWidth(line);

            // Calculate indentation for centering
            int padding = (terminalWidth - displayWidth) / 2;
            if (padding < 0)
                padding = 0; // Prevent negative padding if line is wider than terminal

            // Print spaces for centering
            for (int k = 0; k < padding; k++)
            {
                putchar(' ');
            }

            // Print the (already formatted) line
            printf("%s\n", line);
        }

        // Print blank line between paragraphs, except after the last one
        if (i < doc->paragraphCount - 1)
        {
            printf("\n");
        }
    }
}

/**
 * Reads the content of a file into a dynamically allocated string.
 *
 * @param filename Path to the file to be read.
 * @return Dynamically allocated string with the file content, or NULL on error.
 */
char *readFileToString(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("Error opening file");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    if (fileSize < 0)
    { // Error checking for ftell
        perror("Error determining file size");
        fclose(file);
        return NULL;
    }
    fseek(file, 0, SEEK_SET);

    char *buffer = (char *)malloc(fileSize + 1);
    if (buffer == NULL)
    {
        perror("Memory allocation error for file content buffer");
        fclose(file);
        return NULL;
    }

    size_t bytesRead = fread(buffer, 1, fileSize, file);
    if (bytesRead < fileSize && ferror(file))
    {
        perror("Error reading file");
        fclose(file);
        free(buffer);
        return NULL;
    }
    buffer[bytesRead] = '\0'; // Null-terminate string

    fclose(file);
    return buffer;
}

/**
 * Reads the entire content from Standard Input (stdin) into a dynamically allocated string.
 *
 * @return Dynamically allocated string with the content from stdin, or NULL on error.
 */
char *readStdinToString()
{
    size_t capacity = 1024; // Initial capacity
    size_t size = 0;
    char *buffer = (char *)malloc(capacity);
    if (!buffer)
    {
        perror("Memory allocation error for stdin buffer");
        return NULL;
    }

    size_t bytesRead;
    // Read in chunks until EOF is reached
    while ((bytesRead = fread(buffer + size, 1, capacity - size - 1, stdin)) > 0)
    {
        size += bytesRead;

        // Check if buffer is full (only space left for '\0')
        if (size >= capacity - 1)
        {
            size_t newCapacity = capacity * 2;
            // Check for potential overflow before doubling
            if (newCapacity < capacity)
            {
                fprintf(stderr, "Error: Buffer capacity overflow for stdin.\n");
                free(buffer);
                return NULL;
            }
            char *newBuffer = (char *)realloc(buffer, newCapacity);
            if (!newBuffer)
            {
                perror("Memory reallocation error for stdin buffer");
                free(buffer);
                return NULL;
            }
            buffer = newBuffer;
            capacity = newCapacity;
        }
    }

    // Check for read errors (other than EOF)
    if (ferror(stdin))
    {
        perror("Error reading from stdin");
        free(buffer);
        return NULL;
    }

    buffer[size] = '\0'; // Null-terminate string
    return buffer;
}

int main(int argc, char *argv[])
{
    // Set locale for correct UTF-8 handling
    setlocale(LC_ALL, "");

    char *inputContent = NULL;

    // Decide whether to read from file or stdin
    if (argc == 2)
    {
        // Read from file
        inputContent = readFileToString(argv[1]);
    }
    else if (argc == 1)
    {
        // Read from stdin (pipe)
        inputContent = readStdinToString();
    }
    else
    {
        // Incorrect number of arguments
        fprintf(stderr, "Usage: %s [<filename>]\n", argv[0]);
        fprintf(stderr, "  Reads from <filename> or from standard input if no file is specified.\n");
        return 1;
    }

    // Check if reading was successful
    if (!inputContent)
    {
        // Error message already printed in readFileToString or readStdinToString
        return 1;
    }

    // Parse file content (or stdin content) into a document
    Document *doc = parseDocument(inputContent);
    if (!doc)
    {
        fprintf(stderr, "Error parsing document.\n");
        free(inputContent);
        return 1;
    }

    // Print the document centered
    printCenteredDocument(doc);

    // Cleanup
    freeDocument(doc);
    free(inputContent); // Free the read content

    return 0;
}
