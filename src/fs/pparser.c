#include "pparser.h"
#include "kernel.h"
#include "string/string.h"
#include "memory/memory.h"
#include "memory/heap/kheap.h"
#include "status.h"

//Checks the format and length of the path provided
static uint32_t pathparser_path_valid_format(const char* filename)
{
    uint32_t len = strnlen(filename, CROSOS_MAX_PATH);
    return (len>=3 && isdigit(filename[0]) && memcmp((void*)&filename[1], ":/", 2) == 0); //True if starts with a digit followed by :/
}

//Get drive number of the path, after validating a good path format
//path contains the address of the pointer that points to the absolut path
static uint32_t pathparser_get_drive_by_path(const char** path)
{
    if(!pathparser_path_valid_format(*path)) //Validate the path by passing the address of the absolut path (derefencing one pointer)
    {
        return -EBADPATH;
    }

    uint32_t drive_no = tonumericdigit(*path[0]); //Parses the drive number to an integer

    //Add 3 bytes to skip drive number 0:/
    *path += 3; //Increments by the the content of the pointer to the absolut path.
    return drive_no;
}

//Allocates a root path from a drive number
static struct path_root* pathparser_create_root(uint32_t drive_number)
{
    struct path_root* path_r = kzalloc(sizeof(struct path_root)); //Allocates the needed memory for the root path
    path_r->drive_no = drive_number; //Stores the value of the drive number
    path_r->first = 0; // Temporary value which will further be modified
    return path_r;
}

//Returns the part of a path
static const char* pathparser_get_path_part(const char** path)
{
    char* result_path_part = kzalloc(CROSOS_MAX_PATH); //Allocates space for the path part that will be parsed
    uint32_t i = 0;
    while(**path != '/' && **path != 0x00)
    {
        result_path_part[i] = **path; //Copy the content of the path until a / or a 0x00 is found
        *path += 1;
        i++;
    }

    if(**path == '/')
    {
        //Skip the / to avoid problems on the next part of the path
        *path+=1;
    }

    if(i == 0)
    {
        kfree(result_path_part); //Free the contents of the path part, since we reached the end of the absolut path string
        result_path_part = 0; //Clear the pointer
    }

    return result_path_part;
}

//Creates a path part and linkes it with the previous part
struct path_part* pathparser_parse_path_part(struct path_part* last_part, const char** path)
{
    const char* path_part_str = pathparser_get_path_part(path);
    if(!path_part_str)
    {
        return 0; //End of the path
    }

    struct path_part* part = kzalloc(sizeof(struct path_part));
    part->part = path_part_str; //Initialize the part with the name parsed
    part->next = 0x00; //Temporary next value

    if(last_part)
    {
        last_part->next = part; // Set the last_part next value, the current parsed path
    }

    return part;
}

//Frees the parsed path structures and links from heap
void pathparser_free(struct path_root* root)
{
    struct path_part* part = root->first; //Gets the first part
    while(part) //Repeat until part is null (last part)
    {
        struct path_part* next_part = part->next; //Gets the next part
        kfree((void*) part->part); // Frees the contents on the string of the path part
        kfree(part); // Frees the actual structure
        part = next_part; //References the part variable to the next part address, to repeat the loop
    }

    kfree(root); //Free root part of the path.
}

//Parses an absolut path, getting every folder and file as a path part
struct path_root* pathparser_parse(const char* path, const char* current_directory_path)
{
    uint32_t res = 0; //Temporary return value, checking for inner functions
    const char* tmp_path = path; //Copy of the absolut path provided
    struct path_root* path_root = 0; //Initialization of a path root. Needs to reference the contents parsed
    if(strlen(path) > CROSOS_MAX_PATH)
    {
        goto out; //Length of the absolut path is too big
    }

    res = pathparser_get_drive_by_path(&tmp_path); //Get drive number. It passes the address of the tmp_path, to modify its position without modifying the position of the pointer that points to the actual absolut path we want to parse
    if(res < 0)
    {
        goto out; //Root path is malformed
    }

    path_root = pathparser_create_root(res); //Writes the path_root variable with the drive returned to 'res'
    if(!path_root)
    {
        goto out;
    }

    struct path_part* first_part = pathparser_parse_path_part(NULL, &tmp_path); //Parses first part after the drive. It also passes the addres to tmp_path to play with its position without affecting the original pointer to the aboslut path. Null is provdided because it is the first path part and there is no link to a "previous" path
    if(!first_part)
    {
        goto out;
    }

    path_root->first = first_part; //Links the parsed path with the root path
    struct path_part* part = pathparser_parse_path_part(first_part, &tmp_path); //Since it is the second part of the path, a previous path is provided to link them
    while(part) //While there is a return value on the parse function, there are more and more folders/file
    {
        part = pathparser_parse_path_part(part, &tmp_path); //Parses the path parts until a 0 is find, which means that the end pof the aboslut path string is reached
    }

out:
    return path_root;
}
