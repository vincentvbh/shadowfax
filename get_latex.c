
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>
#include <sys/types.h>
#include <string.h>

char buff[2048];
char *buff_ptr;

static const char *default_in = "bench.txt";
static const char *default_out = "bench_latex.tex";

static size_t sprintf_format_000(char *des, uint32_t perf) {
    size_t len = 0;
    uint32_t local_buff[32];
    char *local_buff_ptr;
    while(perf >= 1000) {
        local_buff[len] = perf % 1000;
        len++;
        perf /= 1000;
    }
    if(perf != 0) {
        local_buff[len] = perf;
        len++;
    }
    local_buff_ptr = des;
    for(ssize_t i = len - 1; i > 0; i--){
        local_buff_ptr += sprintf(local_buff_ptr, "%d\\;", local_buff[i]);
    }
    local_buff_ptr += sprintf(local_buff_ptr, "%d", local_buff[0]);
    *local_buff_ptr = '\0';
    return local_buff_ptr - des;
}

int main(int argc, char **argv) {

    FILE *fp_in, *fp_out;
    char *line = NULL;
    const char *in_name = NULL, *out_name = NULL;
    uint32_t perf;
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
        if(line[0] != '\\')
            continue;
        size_t i;
        for(i = 0; (i < len) && (isdigit(line[i]) == 0); i++) {}
        if(i == len)
            continue;

        sscanf(line + i, "%u", &perf);

        memset(buff, '\0', sizeof(buff));
        memmove(buff, line, i);
        buff_ptr = buff + i;
        buff_ptr += sprintf_format_000(buff_ptr, perf);
        buff_ptr += sprintf(buff_ptr, "}\n");

        fprintf(fp_out, "%s", buff);


    }


}
