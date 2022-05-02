#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int device_write(const char* device_name, const char* data, size_t count)
{
    int fd = open(device_name, O_RDWR);
    if (fd < 0) return -1;
    int bytes = write(fd, data, count);
    int err = close(fd);
    if (err < 0) return -2;
    return bytes;
}

int device_read(const char* device_name, char* data, size_t count)
{
    int fd = open(device_name, O_RDWR);
    if (fd < 0) return -1;
    int bytes = read(fd, data, count);
    int err = close(fd);
    if (err < 0) return -2;
    return bytes;
}

char* random_data(size_t count)
{
    char* ret = malloc(count*sizeof(char));
    for (int i = 0; i < count; ++i)
    {
        ret[i] = rand() % 10;
    }
    return ret;
}

int main(int argc, char** argv)
{
    char* data_write = random_data(10);
    int res_write = device_write("/dev/scull0", data_write, 10);
    printf("written %d bytes\n", res_write);

    char* data_read = malloc(10*sizeof(char));
    int res_read = device_read("/dev/scull0", data_read, 10);
    printf("written %d bytes\n", res_read);

    for (int i = 0; i < 10; ++i)
    {
        printf("w:%d, r:%d\n", data_write[i], data_read[i]);
    }

    return 0;
}