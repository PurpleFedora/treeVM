#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>



/*
This assembler is Work In Progress
*/




enum asm_sections {
	NO_SECTION = 0,
	DATA_SECTION = 1,
	TEXT_SECTION = 2,
};

enum asm_declaration {
	TYPE_NONE = 0,
	TYPE_GLOBAL = 1,
	TYPE_LOCAL = 2,
	TYPE_LABEL = 3,
	TYPE_INSTRUCTION = 4,
};

typedef struct asm_line {
	long int line;		//Real line number, required for debugging
	size_t length;		//Length of the line
	char *p;			//Line
	char *rp;			//Unmodified line
	
	enum asm_sections section;
	struct asm_line *next_section;

	enum asm_declaration type;
} aline_t;
#define LINE_INITIALIZER (aline_t) {0}

enum asm_error {
	ERROR_USAGE = 1,
	ERROR_UNABLE_TO_OPEN_FILE = 2,
	ERROR_OUT_OF_MEMORY = 3,
	ERROR_FTELL_NEGATIVE = 4,
	ERROR_PARSE = 5,
};



static int error_code = 0;
static char * error_message = NULL;
static long int error_line = 0;
	
static FILE *source = NULL;
static char *file = NULL;
static aline_t *line = NULL;
static long int line_len = 0;




int parse_data(aline_t *line) {
	return(1);
}
int parse_text(aline_t *line) {
	return(1);
}



#define ERROR(code, msg) { error_code = code; error_message = msg "\n"; goto error_l; }
#define ERROR_LINE(code, msg, line) { error_code = code; error_message = msg "\n"; error_line = line; goto error_l; }

int main(int argc, char *argv[]) {
	//Test for correct number of arguments
	if (argc < 2) {
		perror("Usage: tree-asm file\n");
		return(ERROR_USAGE);
	}
	
	



	//Read file and parse into array of lines, removes trailing and preceding whitepsace
	{
		source = fopen(argv[1], "rb");
		if (source == NULL)
			ERROR(ERROR_UNABLE_TO_OPEN_FILE, "Unable to open file")
		
		//Get filesize
		fseek(source, 0, SEEK_END);
		unsigned long int fsize = 0;
		{
			long int fsize2 = ftell(source);
			if (fsize2 < 0)
				ERROR(ERROR_FTELL_NEGATIVE, "Ftell Error")
			fsize = (unsigned long int) fsize2;
		}
		fseek(source, 0, SEEK_SET);
		
		
		//Allocate a file buffer
		file = malloc(sizeof(char) * (fsize + 2) * 2);
		if (file == NULL)
			ERROR(ERROR_OUT_OF_MEMORY, "Out of Memory")
		memset(file, 0, sizeof(char) * (fsize + 2));
		
		
		//Allocate line array
		line = malloc(257 * sizeof(aline_t));
		if (line == NULL)
			ERROR(ERROR_OUT_OF_MEMORY, "Out of Memory")
		unsigned long int line_size = 256;
		
		const int max_line_size = (INT_MAX < fsize + 1) ? INT_MAX : (int) fsize + 1;
		
		char *file2 = file;
		char *file3 = file2 + fsize + 2;
		for (long int real_line = 1; fgets(file2, max_line_size, source) != NULL; real_line++) {
			//Read real line
			size_t rlen = strlen(file2);
			strcpy(file3, file2);
			while (rlen > 0 && (file3[rlen - 1] == ' ' || file3[rlen - 1] == '\t' || file3[rlen - 1] == '\r' || file3[rlen - 1] == '\n')) {
				file3[rlen - 1] = '\0';
				rlen--;
			}

			//Remove comment
			char *comment = strchr(file2, ';');
			if (comment != NULL) {
				*comment = '\0';
			}

			size_t len = strlen(file2);
			
			//Remove trailing White space
			while (len > 0 && (file2[len - 1] == ' ' || file2[len - 1] == '\t' || file2[len - 1] == '\r' || file2[len - 1] == '\n')) {
				file2[len - 1] = '\0';
				len--;
			}
			
			if (len == 0)
				continue;

			//Check if the file is empty
			int line_not_empty = 0;
			for (long int i = 0; i < len; i++) {
				switch(file2[i]) {
					case(' '):
					case('\t'): 
					case(';'): continue;
					default: line_not_empty = 1; break;
				}
			}
			
			if (line_not_empty) {
				//Reallocate line array if necessary
				if (line_len + 1 >= line_size) {
					void *tp = realloc(line, (line_size + 1) * sizeof(aline_t));
					if (tp == NULL)
						ERROR(ERROR_OUT_OF_MEMORY, "Out of Memory")
					line = tp;
					line_size *= 2;
				}
				
				//Set values of the new line
				line[line_len] = LINE_INITIALIZER;
				line[line_len].length = len;
				line[line_len].line = real_line;
				line[line_len].p = file2;
				line[line_len].rp = file3;
				

				//Delete whitespace at the start of the line
				while (line[line_len].p[0] == ' ' || line[line_len].p[0] == '\t') {
					line[line_len].p++;
					line[line_len].length--;
				}


				//Increase current line index
				line_len++;
				
				//Advance buffer pointer
				file2 += len + 1;
				file3 += rlen + 1;
			}
		}

		fclose(source); source = NULL;
	}
	
	//Linked list of sections
	aline_t * section_list = NULL;


	//Divide the code into sections
	{
		int section = NO_SECTION;
		int has_section[2] = {0,0};


		aline_t ** last_section = &section_list;
		for (long int i = 0; i < line_len; i++) {
			if (line[i].length == 5) {
				if (strcmp(line[i].p, ".data") == 0) {
					section = DATA_SECTION;
					has_section[0] = 1;
				} else if (strcmp(line[i].p, ".text") == 0) {
					section = TEXT_SECTION;
					has_section[1] = 1;
				} else {
					goto not_valid_section_l;
				}

				line[i].section = NO_SECTION;
				*last_section = &line[i];
				last_section = &line[i].next_section;
				line[i].next_section = NULL;
			} else {
				not_valid_section_l:
				if (section == NO_SECTION) {
					ERROR_LINE(ERROR_PARSE, "Invalid Section", i);
				}

				line[i].section = section;
			}
		}

		line[line_len].section = NO_SECTION;
	}

	//Parse Data and text areas
	{
		aline_t *lsection = section_list;
		while (lsection != NULL) {
			if (lsection[1].section == DATA_SECTION) {
				parse_data(lsection + 1);
			} else {
				parse_text(lsection + 1);
			}
			lsection = lsection->next_section;
		}
	}


	
	for (long int i = 0; i < line_len; i++) {
		printf("I:%.4li R:%.4li L:%.4li %d : \"%s\"\t\t\t\"%s\" \n", i, line[i].line, (long int) line[i].length, line[i].section, line[i].p, line[i].rp);
	}



	error_l:
	if (error_code != 0) {
		if (error_line) {
			char lbuf[17] = {0};
			fprintf(stderr, "Line %li: %s\n", error_line, line[error_line].rp);
		}
		perror(error_message);
	}
	
	
	if (source != NULL)
		fclose(source);
	if (file != NULL)
		free(file);
	if (line != NULL)
		free(line);
	
	return(error_code);
}
