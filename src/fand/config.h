#ifndef CONFIG_H
#define CONFIG_H

enum { DEVICE_PATH_MAX_SIZE = 256 };
enum { MATRIX_MAX_SIZE = 32 };

struct fand_config_data {
    unsigned char matrix_rows;
    unsigned char hysteresis;
    unsigned short interval;
    unsigned char matrix[MATRIX_MAX_SIZE];
    char devpath[DEVICE_PATH_MAX_SIZE];
};

int config_parse(char const *path, struct fand_config_data *data);

#endif /* CONFIG_H */
