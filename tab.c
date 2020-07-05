/*
	TAB.C

	Simple program to add or remove tabs from
	a text file while preserving its spacing.

	Usage:-

	    $ tab [-iN] [-oN] [-d] [-s] input_file output_file
		-iN  sets the input tab size (default is 8)
		-oN  sets the output tab size (default is input -
		     a value of 1 or zero replaces all tabs with spaces)
		-d   process spacing inside double quotes
		-s   process spacing inside single quotes

	For example:-

	    $ tab -o tab.c x.c

	will replace tab characters in the source with appropriate
	numbers of space characters.
*/

#include <stdio.h>
#include <stdlib.h>


int tab(char *in_filename,char *out_filename,int in_size,int out_size,
	int double_q,int single_q)
{
    FILE *in_file,*out_file;
    if (in_size < 1) {
	printf("Input tab size too small (%d)\n",in_size);
	exit(EXIT_FAILURE);
    }
    if (out_size < 1) out_size = in_size;
    if ((in_file = fopen(in_filename,"r")) == NULL) {
	printf("Cannot open input file %s\n",in_filename);
	perror("Input error: ");
    } else {
	if ((out_file = fopen(out_filename,"w")) == NULL) {
	    printf("Cannot open output file %s\n",out_filename);
	    perror("Output error: ");
	} else {
	    int ch,quote_ch = 0;
	    int tab_pos = 0,last_pos = 0;
	    int in_count = 0,out_count = 0,rec_count = 0;
	    while ((ch = fgetc(in_file)) != EOF) {
		int err = 0;
		int ch_width;
		in_count++;
		if (ch >= ' ') {
		    ch_width = 1;
		} else {
		    ch_width = 0;
		    if (ch == '\n' || ch == '\r') {
			tab_pos = 0;
			last_pos = 0;
			quote_ch = 0;
			rec_count++;
		    } else {
			if (ch == '\t')
			    ch_width = (tab_pos / in_size + 1) * in_size - tab_pos;;
		    }
		}
		if (quote_ch || (ch != ' ' && ch != '\t')) {
		    while (last_pos < tab_pos) {
			int tab_skip = (last_pos / out_size + 1) * out_size;
			if (tab_skip - last_pos <= 1 || tab_skip > tab_pos) {
			    err = fputc(' ',out_file);
			    out_count++;
			    last_pos++;
			} else {
			    err = fputc('\t',out_file);
			    out_count++;
			    last_pos = (last_pos / out_size + 1) * out_size;
			}
			if (err == EOF) {
			    printf("I/O error writing %s\n",out_filename);
			    perror("Output error: ");
			    exit(EXIT_FAILURE);
			}
		    }
		    err = fputc(ch,out_file);
		    out_count++;
		    last_pos += ch_width;
		    if (err == EOF) {
			printf("I/O error writing %s\n",out_filename);
			perror("Output error: ");
			exit(EXIT_FAILURE);
		    }
		    if (ch == quote_ch) {
			quote_ch = 0;
		    } else {
			if ((ch == '\"' && double_q == 0) ||
			    (ch == '\'' && single_q == 0)) quote_ch = ch;
		    }
		}
		tab_pos += ch_width;
	    }
	    fclose(out_file);
	    fclose(in_file);
	    printf("Produced %d bytes from %d bytes in %d records\n",
		   in_count,out_count,rec_count);
	    return 1;
	}
    }
    return 0;
}

int main(int argc,char *argv[])
{
    int in_size = 8,out_size = 0,double_q = 0,single_q = 0;
    char *in_filename = NULL,*out_filename = NULL;
    int arg;
    for (arg = 1; arg < argc; arg++) {
	if (*argv[arg] == '-') {
	    switch (*(argv[arg] + 1)) {
		case 'i':
		    in_size = atoi(argv[arg] + 2);
		    break;
		case 'o':
		    out_size = atoi(argv[arg] + 2);
		    if (out_size == 0) out_size = 1;
		    break;
		case 'd':
		    double_q = 1;
		    break;
		case 's':
		    single_q = 1;
		    break;
		default:
		    printf("Don't understand \'%s\'\n",argv[arg]);
		    exit(EXIT_FAILURE);
	    }
	} else {
	    if (in_filename == NULL) {
		in_filename = argv[arg];
	    } else {
		if (out_filename == NULL) {
		    out_filename = argv[arg];
		} else {
		    printf("Too many parameters specified (%s)\n",argv[arg]);
		    exit(EXIT_FAILURE);
		}
	    }
	}
    }
    if (out_filename != NULL) {
	tab(in_filename,out_filename,in_size,out_size,double_q,single_q);
    } else {
	printf("TAB v1.0\n");
	printf("Usage: tab [-iN] [-oN] [-d] [-s] input_file output_file\n");
	printf("  -iN  input tab size is N characters\n");
	printf("  -oN  output tab size is N (defaults to input size)\n");
	printf("  -d   process tabs inside double quotes\n");
	printf("  -s   process tabs inside single quotes\n");
	printf("Setting output tab size to 1 (-o or -o1) will replace tabs with spaces\n");
	printf("Example:\n    tab  -o4 tab.c newtab.c\n");
    }
    return EXIT_SUCCESS;
}
