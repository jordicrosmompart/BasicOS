#ifndef PATH_PARSER_H
#define PATH_PARSER_H
#include <stdint.h>

//Contains the roothard drive of the path and a pointer to the next level
struct path_root
{
    uint32_t drive_no;
    struct path_part* first;
};

//Contains the level name and a pointer to the next level
struct path_part
{
    const char* part;
    struct path_part*  next;
};

struct path_root* pathparser_parse(const char* path, const char* current_directory_path);
void pathparser_free(struct path_root* root);

#endif