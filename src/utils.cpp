#include <getopt.h>
#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <cstring>
#include <vector>
#include <cstdint>
#include <sys/types.h>
#include <sys/stat.h>

#include "utils.h"
#include "input/FileReader.h"
#include "sarray/sarray.h"
#include "compression/lz78.h"
#include "compression/lz77.h"

using namespace std;

program_args::program_args()
  : mode_flag(),
  	compression_flag(LZ78),
    pattern_file(0),
    help_flag(false),
    count_flag(false),
    index_file(0),
    text_file(0) { }

program_args::~program_args() { }

program_args get_program_parameters(int argc, char** argv) {

  int option_index;
  int current_parameter;
  program_args args;
	
  struct option long_options[] =
  {
    {"pattern", 	required_argument, 0, 'p'},
    {"help",    	no_argument,       0, 'h'},
    {"count",   	no_argument,       0, 'c'},
    {"compression", required_argument, 0, 'x'},
    {0, 0, 0, 0}
  };
	
	if (argc > 1) {
		char* mode = argv[1];
		if (strcmp(mode, "index") == 0) {
 			args.mode_flag = Index;
			optind++;
		}
		else if (strcmp(mode, "search") == 0) {
      args.mode_flag = Search;
			optind++;
		}
	}

  while (1) {
    current_parameter = getopt_long(argc, argv, "p:hcx:", long_options, &option_index);

    if (current_parameter == -1) {
      break;
    }

    switch (current_parameter) {
      case 0:
      // No momento, nenhum argumento está sendo usado para setar uma flag
      break;
      case 'p':
      args.pattern_file = optarg;
      break;
      case 'h':
      args.help_flag = true;
      break;
      case 'c':
      args.count_flag = true;
      break;
      case 'x':
      if (strcmp(optarg, "LZ77") == 0) args.compression_flag = LZ77;
      else if (strcmp(optarg, "LZ78") == 0) args.compression_flag = LZ78;
      else { cerr << "Algoritmo de compressão inválido.\n"; exit(1); }
      break;
      case '?':
      // Um argumento desconhecido é apenas ignorado no momento
      break;
      default:
      exit(1);
    }
  }
	
  if (args.mode_flag == Index) {
    if (optind < argc) {
      
      args.text_file = argv[optind++];
    }
  } else if (args.mode_flag == Search) {
    if (optind < argc) {   
      if (!args.pattern_file) {
        args.patterns.push_back(argv[optind++]);
      }
    }
    if (optind < argc) {
      
      args.index_file = argv[optind++];
    }
  }

  return args;
}

void print_help_line(char const *msg1, char const *msg2) {
	cout << left << setw(35) << msg1 << setw(45) << msg2 << endl;
}

void print_help_text() {
  cout << "Usage:" << endl;
  cout << "Index mode: ipmt index textfile" << endl;
  cout << "Options:" << endl;
  cout << "  None" << endl;
  cout << "Search mode: ipmt search pattern indexfile" << endl;
  cout << "Options:" << endl;
  print_help_line("  -c, --count", "Counts the pattern occurrences in the text");
  print_help_line("  -p, --pattern=<pattern file>","Specifies file from which the program should read the patterns to be used (each line of the file specifies a pattern)");
  print_help_line("  -h, --help","Shows this message");
  cout << endl << "  If a pattern file is not specified, the first argument given to pmt will be read as the only pattern to be searched for in the text file. Several source text files can be specified at the same time." << endl;
}

int is_regular_file(const char *path) {
		struct stat path_stat;
		stat(path, &path_stat);
		return S_ISREG(path_stat.st_mode);
}

void read_pattern_file(program_args &args) {
	FileReader fr(args.pattern_file);
	string buffer;
	
	while(fr.hasContent()) {
		buffer = fr.readLine();
	
		if (buffer.size()) {
			args.patterns.push_back(buffer);
		}
	}
}

void search_index_file(program_args &args) {
	
	// decompress
	FILE* fp = fopen(args.index_file, "r");
	uint32_t size;
	uint32_t code_len;
	char* text;
	uint8_t* encoded_byte_array;
	int Ls = (1 << 12) - 1;
	int Ll = (1 << 4) - 1;
	int* sarray;
	int* Llcp;
	int* Rlcp;
	char* byte_array;
	Compression compression_mode;

	fread(&compression_mode, sizeof(int), 1, fp);
	fread(&size, sizeof(uint32_t), 1, fp);
	fread(&code_len, sizeof(uint32_t), 1, fp);

	// Text
	encoded_byte_array = (uint8_t*)malloc(code_len * sizeof(uint8_t));
	fread(encoded_byte_array, sizeof(uint8_t), code_len, fp);
	if (compression_mode == LZ77) {
		text = (char*)malloc(size * sizeof(char));
		lz77_decode(encoded_byte_array, code_len, Ls, Ll, text);
	} else if (compression_mode == LZ78) {
		text = lz78_decode(encoded_byte_array, code_len, size);
	}
	free(encoded_byte_array);

	// Suffix array
	/*
	fread(&code_len, sizeof(uint32_t), 1, fp);
	encoded_byte_array = (uint8_t*)malloc(code_len * sizeof(uint8_t));
	fread(encoded_byte_array, sizeof(uint8_t), code_len, fp);
	byte_array = (char*)malloc((size * 4 * sizeof(char)) + 1);
	lz77_decode(encoded_byte_array, code_len, Ls, Ll, byte_array);
	free(encoded_byte_array);
	sarray = get_int_array_from_bytes(byte_array, size);
	free(byte_array);

	cout << "suffix array decompressed" << endl;
	*/	
	// Llcp

	// Rlcp

	// Loop through patterns and search

}

void create_index_file(program_args &args) {
	FILE* fp = fopen(args.text_file, "r");
	uint32_t size;
	uint32_t code_len;
	char *text;

	/* index file name */
	int source_name_length = (strrchr(args.text_file, '/')-args.text_file+1);
	char *index_name = new char[strlen(args.text_file) - source_name_length + 5];
	memcpy(index_name, &args.text_file[source_name_length], strlen(args.text_file) - source_name_length);
	strcat(index_name, ".idx");

	int* sarray;
	int* Llcp;
	int* Rlcp;
	char* byte_array;
	uint8_t* encoded_byte_array;
	int Ls = (1 << 12) - 1;
	int Ll = (1 << 4) - 1;

	if (fp) {
		fseek(fp, 0, SEEK_END);
		size = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		text = (char*)malloc((size + 1)*sizeof(char));
		fread(text, sizeof(char), size, fp);
		text[size] = 0;
		fclose(fp);

		/* ************ */
		//encoded_byte_array = 
		/*uint8_t* code = lz78_encode(text, size, &code_len);
		cout << "code len: " << code_len << endl;
		char* rec_text = lz78_decode(code, code_len, size);
		//cout << "cod: " << encoded_byte_array << endl;
		//lz78_decode(encoded_byte_array, code_len);
		cout << "texto: " << rec_text << endl;
		free(encoded_byte_array);*/
		/* ************ */

		build_sarray_LRlcp(text, size, &sarray, &Llcp, &Rlcp);

		fp = fopen(index_name, "wb+");

		if (fp) {

			fwrite(&args.compression_flag, sizeof(int), 1, fp);

			if (args.compression_flag == LZ77)
				encoded_byte_array = lz77_encode(text, size, Ls, Ll, &code_len);
			else if (args.compression_flag == LZ78)
				encoded_byte_array = lz78_encode(text, size, &code_len);
			
			fwrite(&size, sizeof(uint32_t), 1, fp);
			fwrite(&code_len, sizeof(uint32_t), 1, fp);
			fwrite(encoded_byte_array, sizeof(uint8_t), code_len, fp);
			free(encoded_byte_array);


			byte_array = get_bytes_from_array(sarray, size);
			free(sarray);
			if (args.compression_flag == LZ77)
				encoded_byte_array = lz77_encode(byte_array, size * 4, Ls, Ll, &code_len);
			else if (args.compression_flag == LZ78)
				encoded_byte_array = lz78_encode(byte_array, size * 4, &code_len);
			free(byte_array);
			fwrite(&code_len, sizeof(uint32_t), 1, fp);
			fwrite(encoded_byte_array, sizeof(uint8_t), code_len, fp);
			free(encoded_byte_array);

			byte_array = get_bytes_from_array(Llcp, size);
			free(Llcp);
			if (args.compression_flag == LZ77)
				encoded_byte_array = lz77_encode(byte_array, size * 4, Ls, Ll, &code_len);
			else if (args.compression_flag == LZ78)
				encoded_byte_array = lz78_encode(byte_array, size * 4, &code_len);
			free(byte_array);
			fwrite(&code_len, sizeof(uint32_t), 1, fp);
			fwrite(encoded_byte_array, sizeof(uint8_t), code_len, fp);
			free(encoded_byte_array);

			byte_array = get_bytes_from_array(Rlcp, size);
			free(Rlcp);
			if (args.compression_flag == LZ77)
				encoded_byte_array = lz77_encode(byte_array, size * 4, Ls, Ll, &code_len);
			else if (args.compression_flag == LZ78)
				encoded_byte_array = lz78_encode(byte_array, size * 4, &code_len);
			free(byte_array);
			fwrite(&code_len, sizeof(uint32_t), 1, fp);
			fwrite(encoded_byte_array, sizeof(uint8_t), code_len, fp);
			free(encoded_byte_array);
			
			fclose(fp);
		} else {
			printf("Erro ao abrir o arquivo %s\n", index_name);
			exit(1);
		}

	} else {
		printf("Erro ao abrir o arquivo %s\n", args.text_file);
		exit(1);
	}
}
