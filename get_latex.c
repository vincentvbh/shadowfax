
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/types.h>

static const char *default_in = "bench.txt";
static const char *default_out = "bench_latex.tex";

int main(int argc, char **argv) {

    FILE *fp_in, *fp_out;
    char *line = NULL;
    const char *in_name = NULL, *out_name = NULL;
    size_t len = 0;
    ssize_t read;

    if(argc != 3) {
        in_name = default_in;
        out_name = default_out;
    }else{
        in_name = argv[1];
        out_name = argv[2];
    }

    fp_in = fopen(in_name, "r");
    if(fp_in == NULL){
        fprintf(stderr, "fp_in is NULL!\n");
        exit(EXIT_FAILURE);
    }
    fp_out = fopen(out_name, "w");
    if(fp_out == NULL){
        fprintf(stderr, "fp_out is NULL!\n");
        exit(EXIT_FAILURE);
    }

    while ((read = getline(&line, &len, fp_in)) != -1) {
        if(line[0] == '\\')
            fprintf(fp_out, "%s", line);
    }


}
