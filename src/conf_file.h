/*
 * This file is part of the MAVLink Router project
 *
 * Copyright (C) 2017  Intel Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#define DECLARE_PARSE_INT(_type) \
    static int parse_##_type(const char *val, size_t val_len, void *storage, size_t storage_len)

/**
 * Load and parse multiple conf files, offering methods to extract the configuration options to user
 * structs.
 */
class ConfFile {
public:
    ConfFile()
        : _files(nullptr)
        , _sections(nullptr){};
    ~ConfFile();

    /**
     * Table with information on how to parse options from conf file.
     *
     * @param key The key to the option field
     * @param required If the field is required, extract_options() will fail if it is not found
     * @param parser_func Function to parse the value of the desired option, where val is the
     * pointer to value as string, val_len is size of the value, storage is a pointer to the area to
     * store the parsed value and storage_len is the length of the storage area.
     * @storage_offset The offset from the beggining of the struct used in extract_options() to
     * store the values.
     * @storage_len The length in bytes of the storage poited by @a storage_offset.
     */
    struct OptionsTable {
        const char *key;
        bool required;
        int (*parser_func)(const char *val, size_t val_len, void *storage, size_t storage_len);
        off_t storage_offset;
        size_t storage_len;
    };

    /**
     * Load and parse a configuration file.
     *
     * Open the file using mmap and parse it, keeping an internal structure with pointers to conf
     * sections and options, in order to do a quick extraction using extract_options().
     * When an option in a section is already present before parse is called, the option is
     * overrided by the option from the new file. Other options from sections are not changed.
     * Files are kept open until release_all() is called or the ConfFile object is destructed.
     *
     * @param filename The conf filename
     * @return errno on IO or parsing errors or @c 0 if successful
     */
    int parse(const char *filename);

    /**
     * Release all opened files and internal structures from this ConfFile.
     */
    void release_all();

    /**
     * Extract options set in @a table from section @a section_name
     *
     * @param section_name Name of the section to extract options.
     * @param table Array of TableOption with information on which fields are going to be
     * extracted and how to extract them.
     * @param table_len The number of elements in @a table.
     * @param data A pointer to the struct that will be used to hold the extracted data.
     */
    int extract_options(const char *section_name, const OptionsTable table[], size_t table_len,
                        void *data);
    /**
     * Loop to all sub-sections from the section @a section_name
     *
     * @param section_name The name of the section to look for sub-sections
     * @param separator Charatecter that separates a section from the sub-section
     * @param sub_name A pointer to a char* that will be updated with current sub-section name
     * @param sub_len A pointer to size_t element be filled with the length of the current
     * sub-section name.
     * @param table Array of TableOption with information on which fields are going to be
     * extracted and how to extract them.
     * @param table_len The number of elements in @a table.
     * @param data A pointer to the struct that will be used to hold the extracted data.
     * @param ptr A void pointer to be used internally in the loop. Must be initialized with @c NULL
     * for first execution.
     */
    int loop_sub_sections(const char *section_name, char separator, const char **sub_name,
                          size_t *sub_len, const OptionsTable table[], size_t table_len, void *data,
                          void **ptr);

    // Helpers
    static int parse_bool(const char *val, size_t val_len, void *storage, size_t storage_len);
    static int parse_str_dup(const char *val, size_t val_len, void *storage, size_t storage_len);
    static int parse_str_buf(const char *val, size_t val_len, void *storage, size_t storage_len);
    DECLARE_PARSE_INT(i);
    DECLARE_PARSE_INT(ul);
    DECLARE_PARSE_INT(ull);

private:
    struct conffile *_files;
    struct section *_sections;

    int _parse_file(const char *addr, size_t len, const char *filename);
    struct section *_find_section(const char *section_name, size_t len);
    struct config *_find_config(struct section *s, const char *key_name, size_t key_len);

    struct section *_add_section(const char *addr, size_t len, int line, const char *filename);
    int _add_config(struct section *s, const char *entry, size_t entry_len, const char *filename,
                    int line);
    void _trim(const char **str, size_t *len);
    int _extract_options_from_section(struct section *s, const OptionsTable table[],
                                      size_t table_len, void *data);
};

/*
 * Return the number of elements in a OptionTable array.
 */
#define OPTIONS_TABLE_SIZE(_table) (sizeof(_table) / sizeof(ConfFile::OptionsTable))

/*
 * Create the last two elements in OptionsTable, storage_offset and storage_len, using _field
 * from _struct.
 */
#define OPTIONS_TABLE_STRUCT_FIELD(_struct, _field) \
    offsetof(_struct, _field), sizeof(_struct::_field)

#undef DECLARE_PARSE_INT
