#ifdef __cplusplus
extern "C" {
#endif

#ifndef READ_CONFIG_H
#define READ_CONFIG_H

#include <default.h>

typedef struct {
   char *content;
   uint64_t size;
   uint64_t count;
} nn_string_stream_t;

typedef struct {
   nn_string_stream_t **content;
   uint64_t size;
   uint64_t count;
} nn_string_container_t;

nn_string_stream_t *nn_write_to_sstream(nn_string_stream_t *restrict str, const char *restrict stuff, size_t n);
int nn_write_sstream_to_file(nn_string_stream_t *restrict str, const char *restrict file_path);
size_t nn_write_sstream_to_file_handler(nn_string_stream_t *restrict str, FILE *restrict fp);
bool nn_compare_sstream_to_char(nn_string_stream_t *restrict str, const char *restrict buff);
bool nn_compare_sstream(nn_string_stream_t *restrict str1, nn_string_stream_t *restrict str2);
nn_string_stream_t *nn_read_file_to_sstream(const char *restrict file_path);
nn_string_container_t *nn_tokenize_sstream(nn_string_stream_t *restrict str, const char delim[]);

void nn_free_sstream(nn_string_stream_t **str);
void nn_clear_sstream(nn_string_stream_t *str);
void nn_free_scontainer(nn_string_container_t **tokens);
void nn_free_content_scontainer(nn_string_container_t *tokens);

typedef enum { INTEGER = 0, FLOAT, EXPONENTIAL, STRING } nn_input_format_e;

void nn_reset_read_config();
void nn_conf_append_option(const char options_local[], nn_input_format_e fmt);
int nn_read_configuration_file_sstream(void *(dc[]), const char fn[]);

#endif
#ifdef __cplusplus
}
#endif
