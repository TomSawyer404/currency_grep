#include "../include/concurrency_grep.h"

// static void grep_run(char* file_pointer, char* pattern, u_int32_t file_size);
static void grep_run(Config* my_config);
static void* work_func(void* arg);
static int kmp(const char* str, const char* pat, int32_t* prefix_table);
static int* build_prefix_table(const char* str_pat, int len_pat);
static Config* config_new(char* file_pointer, char* pattern,
                          u_int32_t file_size);

const char* RED = "\x1b[31m";
const char* RESET = "\x1b[0m";
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

/// Create a new config_st
static Config* config_new(char* fp, char* pattern, u_int32_t file_size) {
    if (NULL == fp || NULL == pattern) {
        fprintf(stderr, "NULL pointer with file_pointer!\n");
        return NULL;
    }

    Config* new_config = calloc(0x1, sizeof(Config));
    if (NULL == new_config) {
        perror("calloc() in config_new()");
        return NULL;
    }

    // Split file according to number of CPU
    int cpu_num = 1;
    if (file_size > 4096) {
        cpu_num = get_nprocs();
        if (cpu_num <= 0) {
            perror("get_nprocs()");
            return NULL;
        }
    }
    u_int32_t unit = file_size / cpu_num;

    // Construct prefix_table for pattern
    u_int32_t len_pattarn = strlen(pattern);
    new_config->prefix_table = malloc(sizeof(int32_t) * len_pattarn);
    if (NULL == new_config->prefix_table) {
        perror("malloc() in config_new()");
        return NULL;
    }
    new_config->prefix_table = build_prefix_table(pattern, len_pattarn);

    new_config->file_pointer = fp;
    new_config->pattern = pattern;
    new_config->cpu_num = cpu_num;
    new_config->unit = unit;
    new_config->file_size = file_size;
    new_config->seq = 0;

    return new_config;
}

/// Initialize grep envirmentation, including:
/// 1. Check argv[];
/// 2. Open argv[2] file, and get file size;
/// 3. Create a mmap();
/// 4. Run the `grep` and unmmap();
int grep_init(char** argv) {
    // 1. Check argv
    if (strcmp("", argv[1]) == 0 || strcmp("", argv[2]) == 0) {
        fprintf(stderr, "Empty string in pattern or file!\n");
        return -1;
    }

    // 2. Open a file and figure out how long is it
    int fd = open(argv[2], O_RDWR);
    if (fd < 0) {
        perror("open()");
        return -1;
    }
    struct stat file_stat_st;
    if (fstat(fd, &file_stat_st) < 0) {
        perror("fstat()");
        exit(1);
    }

    // 3. Map file to process's address space
    char* ret = mmap(NULL, file_stat_st.st_size, PROT_READ | PROT_WRITE,
                     MAP_SHARED, fd, 0);
    if (ret == MAP_FAILED) {
        perror("mmap()");
        return -1;
    }
    close(fd);

    // 4. Find pattern
    Config* my_config = config_new(ret, argv[1], file_stat_st.st_size);
    if (NULL == my_config) {
        fprintf(stderr, "Error in config_new()\n");
        return -1;
    }
    grep_run(my_config);

    munmap(ret, file_stat_st.st_size);
    return 0;
}

static void grep_run(Config* my_config) {
    // Deal with every unit
    u_int32_t cpu_num = my_config->cpu_num;
    pthread_t tid[cpu_num];
    for (int i = 0; i < cpu_num; i += 1) {
        my_config->seq = i;

        if (cpu_num - 1 == i) {
            my_config->unit += my_config->file_size % cpu_num;
        }
        pthread_create(tid + i, NULL, work_func, my_config);
    }

    // Wait child thread
    for (int i = 0; i < cpu_num; i += 1) {
        pthread_join(tid[i], NULL);
    }
}

/// Initialize configuration
static void* work_func(void* arg) {
    pthread_mutex_lock(&g_mutex);
    Config* my_config = (Config*)arg;
    pthread_mutex_unlock(&g_mutex);

    // Search range: [ offset * uint, (offset+1) * unit ]
    u_int32_t unit = my_config->unit;
    u_int32_t start_offset = my_config->seq * unit;
    char* src = my_config->file_pointer + start_offset;
    char* dst = src + unit;

    char* sol = src;                // start of line
    char* eol = strchr(src, '\n');  // end of line
    if (NULL != eol) *eol = '\0';
    while (eol && eol <= dst && sol <= dst) {
        if (-1 != kmp(sol, my_config->pattern, my_config->prefix_table)) {
            printf("%s\n", sol);
        }

        *eol = '\n';
        sol = eol + 1;
        eol = strchr(sol, '\n');
        if (NULL != eol) *eol = '\0';
    }

    pthread_exit(NULL);
}

/// A prefix table of pattern string is needed by KMP algorithm
static int* build_prefix_table(const char* str_pat, int len_pat) {
    int* prefix_table = (int*)calloc(1, sizeof(int) * len_pat);
    int* tmp_table = (int*)calloc(1, sizeof(int) * len_pat);
    if (NULL == prefix_table || NULL == tmp_table) {
        perror("calloc()");
        exit(1);
    }

    int i = 0, j = 1;
    tmp_table[0] = 0;
    while (j < len_pat) {  // O(len_pat)
        if (i == 0 && str_pat[i] != str_pat[j]) {
            tmp_table[j] = 0;
            j++;
        } else if (str_pat[i] == str_pat[j]) {
            tmp_table[j] = i + 1;
            i++;
            j++;
        } else {
            i = tmp_table[i - 1];
        }
    }

    // 把tmp_table抄录到prefix_table, 注意首项值为-1
    prefix_table[0] = -1;
    for (int i = 1; i < len_pat; i++)  // O(len_pat)
        prefix_table[i] = tmp_table[i - 1];

    free(tmp_table);
    return prefix_table;
}

/// Return first index of pattern in text.
/// Return -1 if pattern not found.
static int kmp(const char* text, const char* pattern, int32_t* prefix_table) {
    int len_str = strlen(text);     // O(len_str)
    int len_pat = strlen(pattern);  // O(len_pat)
    if (len_pat > len_str) {
        // fprintf(stderr, "len_pat > len_str\n");
        return -1;
    }

    int index_text = 0, index_pattern = 0;
    // int* prefix_table = build_prefix_table(pattern, len_pat);  // Tm = O(?)
    while (index_text < len_str && index_pattern < len_pat) {  // O(len_pat)
        if (text[index_text] == pattern[index_pattern]) {
            index_text += 1;
            index_pattern += 1;
        } else if (text[index_text] != pattern[index_pattern] &&
                   prefix_table[index_pattern] != -1) {
            index_pattern = prefix_table[index_pattern];
        } else {
            index_text += 1;
        }
    }

    if (index_pattern == len_pat) return index_text - len_pat;
    return -1;
}
