#ifdef __cplusplus
extern "C" {
#endif

#include <read_config.h>

#define CHECK_NOT_NUM(tk, i) ((tk)[i] < '0' || (tk)[i] > '9')

// Print C string to string stream up to 'n' char. If str==NULL, a new string stream is initialized and returned.
nn_string_stream_t *nn_write_to_sstream(nn_string_stream_t *restrict str, const char *restrict stuff, size_t n)
{
   if (NULL == str) {
      DA_INIT(str, nn_string_stream_t, char);
   }

   size_t len = strlen(stuff);
   len = (n > len ? len : n);
   DA_APPEND_MANY(str, stuff, len);

   return str;
}

// Utility function, allows to print directly to standard pipes.
size_t nn_write_sstream_to_file_handler(nn_string_stream_t *restrict str, FILE *restrict fp)
{

   if (NULL == str) nn_log(NN_ERROR, "Trying to print NULL string stream to file.");
   if (NULL == str->content || 0 == str->size) nn_log(NN_ERROR, "Trying to print NULL string stream / zero-sized string stream content to file.");

   return fwrite(str->content, 1, str->count, fp);
}

// Print string stream to given file. Performs checks on validity of file and stringstream
int nn_write_sstream_to_file(nn_string_stream_t *restrict str, const char *restrict file_path)
{
   FILE *fp = fopen(file_path, "rb");
   if (!fp) nn_log(NN_ERROR, "Impossible to open the file <%s>", file_path);

   size_t n_char = nn_write_sstream_to_file_handler(str, fp);

   fclose(fp);

   if (n_char != str->count) nn_log(NN_ERROR, "Wrote %ld character to file %s where count of string stream is %ld", n_char, file_path, str->count);

   return 0;
}

// Binary read the entire file content into a char string stream
nn_string_stream_t *nn_read_file_to_sstream(const char *restrict file_path)
{
   FILE *fp = fopen(file_path, "rb");
   if (!fp) nn_log(NN_ERROR, "Impossible to open the file <%s>", file_path);

   // Get file size
   fseek(fp, 0L, SEEK_END);
   long file_size = ftell(fp);
   // nn_log(nn_INFO, "Read file %s with size: %ld", file_path, file_size);
   rewind(fp); // back to beginning of file

   int64_t chunks = (int64_t)file_size / ((int64_t)DA_INCREMENT);

   nn_string_stream_t *sstream = (nn_string_stream_t *)calloc(1, sizeof(nn_string_stream_t));
   // add extra increment to capture the remainder of the integers division and to ensure blocks of DA_INCREMENT size
   sstream->size = chunks * DA_INCREMENT + DA_INCREMENT;
   sstream->count = file_size;
   sstream->content = (char *)calloc(sstream->size, sizeof(char));

   // here assume that fread does not append extra null-termination (shoudl not)
   size_t n_char = fread(sstream->content, 1, file_size, fp);
   if (n_char != (size_t)file_size) nn_log(NN_ERROR, "Read %ld character where size of file <%s> is %ld", n_char, file_path, file_size);
   fclose(fp);
   return sstream;
}

// Compare content of the string stream to the content of char buffer. The '\0' character in buff is ignored
bool nn_compare_sstream_to_char(nn_string_stream_t *restrict str, const char *restrict buff)
{
   // Not worth it to compute strlen since it traverse the whole char array anyway, so it is better to just loop once.
   bool are_equal = true;
   size_t i = 0;
   while (are_equal) {
      are_equal = are_equal && (str->content[i] == buff[i]);
      i++;
      if (i == str->count && buff[i] != 0) are_equal = false;
      if (buff[i] == 0 && i != str->count) are_equal = false;
      if (i == str->count && buff[i] == 0) break;
   }
   return are_equal;
}

bool nn_compare_sstream(nn_string_stream_t *restrict str1, nn_string_stream_t *restrict str2)
{
   bool are_equal = true;
   if (str1->count != str2->count) return false;

   for (size_t i = 0; i < str1->count; i++) {
      are_equal = are_equal && (str1->content[i] == str2->content[i]);
      if (!are_equal) break;
   }
   return are_equal;
}

// Split a given string stream into separate tokens in one pass based on the delimiters 'delim'
nn_string_container_t *nn_tokenize_sstream(nn_string_stream_t *restrict str, const char delim[])
{
   if (!str) nn_log(NN_ERROR, "Trying to tokenize null string stream");
   if (!str->content || 0 == str->size) nn_log(NN_ERROR, "Trying to tokenize null string stream content");
   size_t n_del = strlen(delim);
   nn_string_container_t *tokens = NULL;
   // Init the 'tokens' container and alloc 1024 pointers for the content
   DA_INIT(tokens, nn_string_container_t, nn_string_stream_t *);
   { // In multiple steps to avoid having to check for out of bound here when I have the macro to do it
      nn_string_stream_t *temp = NULL;
      DA_INIT(temp, nn_string_stream_t, char);
      DA_APPEND(tokens, temp);
   }

   bool skipping = true;

   for (size_t i = 0; i < str->count; i++) {
      bool split = false;
      for (size_t j = 0; j < n_del; j++) {
         split = split || (str->content[i] == delim[j]);
         if (split) break;
      }

      if (split && skipping) continue;
      if (split && !skipping) {
         nn_string_stream_t *temp = NULL;
         DA_INIT(temp, nn_string_stream_t, char);
         DA_APPEND(tokens, temp);
         skipping = true;
      }
      if (!split) {
         DA_APPEND(tokens->content[tokens->count - 1], str->content[i]);
         skipping = false;
      }
   }

   // Clear last token if null
   if (tokens->content[tokens->count - 1]->count == 0) {
      nn_free_sstream(&(tokens->content[tokens->count - 1]));
      tokens->count--;
   }
   return tokens;
}

// Free the string stream
void nn_free_sstream(nn_string_stream_t **str)
{
   if (NULL == (*str)) nn_log(NN_ERROR, "Trying to free NULL string stream");
   if (NULL == (*str)->content && (*str)->size != 0) nn_log(NN_ERROR, "Trying to free NULL string stream content when size is not zero.");

   if ((*str)->size > 0) free((*str)->content);
   free((*str));
   (*str) = NULL;

   return;
}

// Clear string stream content
void nn_clear_sstream(nn_string_stream_t *str)
{
   if (NULL == str) nn_log(NN_ERROR, "Trying to clear NULL string stream");
   if (NULL == str->content && str->size != 0) nn_log(NN_ERROR, "Trying to clear NULL string stream content when size is not zero.");

   if (str->size > 0) free(str->content);
   str->content = NULL;
   DA_INIT(str, nn_string_stream_t, char);
   free(str);

   return;
}

void nn_free_scontainer(nn_string_container_t **tokens)
{
   if (NULL == (*tokens)) nn_log(NN_ERROR, "Trying to free NULL string container.");
   if (NULL == (*tokens)->content && (*tokens)->size != 0)
      nn_log(NN_ERROR, "Trying to clear NULL string container content when size is not zero.");
   if ((*tokens)->size > 0) {
      for (size_t i = 0; i < (*tokens)->count; i++)
         nn_free_sstream(&((*tokens)->content[i]));
      free((*tokens)->content);
      free((*tokens));
      (*tokens) = NULL;
   }
}

// To be used with stack-allocated tokens
void nn_free_content_scontainer(nn_string_container_t *tokens)
{
   if (NULL == tokens) nn_log(NN_ERROR, "Trying to free NULL string container.");
   if (NULL == tokens->content && 0 != tokens->size) nn_log(NN_ERROR, "Trying to clear NULL string container content when size is not zero.");
   if (tokens->size > 0) {
      for (size_t i = 0; i < tokens->count; i++)
         nn_free_sstream(&(tokens->content[i]));
      free(tokens->content);
      tokens->content = NULL;
   }
}

#define CLEAR_LOCAL_VAR                                                                                                                              \
   {                                                                                                                                                 \
      if (NULL != (options)) {                                                                                                                       \
         nn_free_scontainer(&(options));                                                                                                             \
      }                                                                                                                                              \
      if (NULL != options_types) free(options_types);                                                                                                \
      options_types = NULL;                                                                                                                          \
      if (NULL != found_option) free(found_option);                                                                                                  \
      found_option = NULL;                                                                                                                           \
   }

// TODO: for the moment this works but is legacy stuff, to be replaced by string streams

static nn_string_container_t *options = NULL;
static nn_input_format_e *options_types = NULL;
static bool *found_option = NULL;

// static size_t N_OPT = 0;
// static char **options = NULL;
// static nn_input_format_e *options_types = NULL;
// static bool *found_option = NULL;
// --

void nn_reset_read_config() { CLEAR_LOCAL_VAR; }

void nn_conf_append_option(const char options_local[], nn_input_format_e fmt)
{
   // Init the options to empty container
   if (NULL == options) {
      DA_INIT(options, nn_string_container_t, nn_string_stream_t *);
   }

   // Append empty element to the container
   {
      // Append first NULL string stream
      nn_string_stream_t *temp = NULL;
      DA_INIT(temp, nn_string_stream_t, char);
      DA_APPEND(options, temp);

      found_option = (bool *)realloc(found_option, sizeof(bool) * ((options)->count));
      options_types = (nn_input_format_e *)realloc(options_types, ((options)->count) * sizeof(nn_input_format_e));
   }

   size_t current = options->count - 1;

   options_types[current] = fmt;
   found_option[current] = false;
   options->content[current] = nn_write_to_sstream(options->content[current], options_local, strlen(options_local));
   STR_APPEND_NULL(options->content[current]);
}

static int8_t nn_check_number_nonsense(size_t opt, nn_string_stream_t *tk, nn_input_format_e fmt)
{
   // Check whether a given token has the correct number format
   switch (fmt) {
   case INTEGER: {
      for (size_t i = 0; i < tk->count; i++) {
         if (tk->content[i] == '\0') continue;
         if (CHECK_NOT_NUM(tk->content, i) && tk->content[0] != '+') {
            return 1;
         }
      }
      break;
   }
   case FLOAT: {

      bool dot = 0;
      if (CHECK_NOT_NUM(tk->content, 0) && tk->content[0] != '+' && tk->content[0] != '-') {
         if (tk->content[0] != '.') {
            return 1;
         } else {
            dot = true;
         }
      }
      for (size_t i = 1; i < tk->count; i++) {
         if (tk->content[i] == '\0') continue;
         if (CHECK_NOT_NUM(tk->content, i)) {
            if (tk->content[i] != '.') {
               return 1;
            } else if (!dot) {
               dot = true;
            } else {
               return 1;
            }
         }
      }
      break;
   }
   case EXPONENTIAL: {
      size_t l = tk->count;
      bool dot = false;
      if (CHECK_NOT_NUM(tk->content, 0) && tk->content[0] != '+' && tk->content[0] != '-') {
         if (tk->content[0] != '.') {
            return 1;
         } else {
            dot = true;
         }
      }
      bool e = false;
      bool sign = false;
      for (size_t i = 1; i < l; i++) {
         if (tk->content[i] == '\0') continue;
         if (CHECK_NOT_NUM(tk->content, i)) {
            if (tk->content[i] != '.' && tk->content[i] != 'e' && tk->content[i] != 'E' && tk->content[i] != '+' && tk->content[i] != '-') {
               return 1;
            } else {
               switch (tk->content[i]) {
               case '.': {
                  if (!dot) {
                     dot = true;
                     break;
                  } else goto error_goto;
                  break;
               }
               case '+': {
                  if (!sign) {
                     sign = true;
                     break;
                  } else goto error_goto;
                  break;
               }
               case '-': {
                  if (!sign) {
                     sign = true;
                     break;
                  } else goto error_goto;
                  break;
               }
               case 'e': {
                  if (!e) {
                     e = true;
                     break;
                  } else goto error_goto;
                  break;
               }
               case 'E': {
                  if (!e) {
                     e = true;
                     break;
                  } else goto error_goto;
                  break;
               }
               default: {
               error_goto:
                  return 1;
               }
               }
            }
         }
      }
      break;
   }
   default:
      break;
   }
   return 0;
}

static void nn_fill_variable(size_t opt, nn_string_stream_t *tk, void *value)
{
   if (opt >= options->count) nn_log(NN_ERROR, "Out of bound option.");

   int8_t is_not_ok = 0;
   if (options_types[opt] == FLOAT || options_types[opt] == EXPONENTIAL) {
      is_not_ok = nn_check_number_nonsense(opt, tk, FLOAT);
      is_not_ok = is_not_ok && nn_check_number_nonsense(opt, tk, EXPONENTIAL);
      if (is_not_ok) {
         nn_log(NN_INFO,
                "Something wrong in the format for %s. Expected a "
                "floating point number, instead found <<%s>>",
                options->content[opt]->content, tk->content);
         nn_log(NN_INFO, "Resorting to default value, see `config.out\'");
         return;
      }

   } else if (options_types[opt] == INTEGER) {
      is_not_ok = nn_check_number_nonsense(opt, tk, INTEGER);
      if (is_not_ok) {
         nn_log(NN_INFO,
                "Something wrong in the format for %s. Expected an "
                "integer number, instead found <<%s>>",
                options->content[opt]->content, tk->content);
         nn_log(NN_INFO, "Resorting to default value, see `config.out\'");
         return;
      }
   }

   found_option[opt] = true;

   switch (options_types[opt]) {
   case INTEGER: {
      int32_t *location = (int32_t *)value;
      if (NULL == location) nn_log(NN_ERROR, "Trying to write integer to NULL location.");
      *location = atoi(tk->content);
      break;
   }
   case FLOAT: {
      double *location = (double *)value;
      if (NULL == location) nn_log(NN_ERROR, "Trying to write float to NULL location.");
      *location = strtod(tk->content, NULL);
      break;
   }
   case EXPONENTIAL: {
      double *location = (double *)value;
      if (NULL == location) nn_log(NN_ERROR, "Trying to write exponential float to NULL location.");
      *location = strtod(tk->content, NULL);
      // *(double *)value = strtod(tk->content, NULL);
      break;
   }
   case STRING: {
      // strncpy((char *)value, tk, BUFF_LIM - 1);
      char *temp = (char *)value;
      if (NULL == temp) nn_log(NN_ERROR, "Trying to write string to NULL location.");
      // this should provide check of out-of-bound memory write
      size_t how_many = min2((uint64_t)(BASEFOLDER_MAX_L + 1), tk->count);
      if (how_many != tk->count) nn_log(NN_WARNING, "Cutting token to fit in standard string size. Please check if this is intended behavior.");
      // NULL character already in the token!
      for (size_t i = 0; i < how_many; i++)
         temp[i] = tk->content[i];

      break;
   }
   default:
      break;
   }

   return;
}

static int8_t nn_check_for_skipping_in_sstream(nn_string_stream_t *str)
{
   for (size_t i = 0; i < str->count; i++) {
      if (str->content[i] == '\n' || str->content[i] == '#' || str->content[i] == '\0') return 2;
      if (str->content[i] == ' ' || str->content[i] == '\t') {
         // Moving the pointer forward and at the same time decreasing the count
         (str->content)++;
         str->count--;
         i++;
      } else {
         return 0;
      }
   }
   return 1;
}

int nn_read_configuration_file_sstream(void *(dc[]), const char fn[])
{
   nn_string_stream_t *full_file = nn_read_file_to_sstream(fn);
   nn_string_container_t *lines = nn_tokenize_sstream(full_file, "\n");
   nn_free_sstream(&full_file);
   char delim[24] = " ,;\n\t=";

   for (size_t i = 0; i < lines->count; i++) {
      // buffer = buffer_in;
      nn_string_stream_t *line = lines->content[i];
      // Check for skipping characters
      if (nn_check_for_skipping_in_sstream(line) != 0) continue;

      for (size_t j = 0; j < line->count; j++) {
         if (line->content[j] == '#') line->count = j;
      }

      nn_string_container_t *tokens = nn_tokenize_sstream(line, delim);
      STR_APPEND_NULL(tokens->content[0]);
      if (tokens->count <= 1) {
         nn_log(NN_WARNING, "In reading the configuration file `%s\': empty token value <<%s>>", fn, tokens->content[0]->content);
         nn_free_scontainer(&tokens);
         continue;
      }

      size_t opt = 0;
      for (opt = 0; opt < options->count; opt++) {
         if (nn_compare_sstream(tokens->content[0], options->content[opt])) break;
      }

      if (opt >= options->count) {
         nn_log(NN_WARNING, "In reading the configuration file `%s\': unrecognize token <<%s>>", fn, tokens->content[0]->content);
         nn_free_scontainer(&tokens);
         continue;
      }

      STR_APPEND_NULL(tokens->content[1]);

      nn_fill_variable(opt, tokens->content[1], dc[opt]);

      for (size_t j = 2; j < tokens->count; j++) {
         STR_APPEND_NULL(tokens->content[j]);
         nn_log(NN_WARNING, "Ignored tokens %s", tokens->content[j]->content);
      }
      nn_free_scontainer(&tokens);
   }
   for (size_t i = 0; i < options->count; i++) {
      if (!found_option[i]) {
         STR_APPEND_NULL(options->content[i]);
         nn_log(NN_WARNING, "Option not found or incorrect format: %s", options->content[i]->content);
      }
   }

   return 0;
}

#ifdef __cplusplus
}
#endif
