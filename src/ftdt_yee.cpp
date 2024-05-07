#include <iostream>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <vector>
#include <array>
#include <thread>
#include <mutex>


// Taille de la matrice de travail (un côté)
static const int size_x = 100;
static const int size_y = 100;
static const int size_z = 100;
static const int MATRIX_SIZE = 100;
static const int BUFFER_SIZE = size_x * size_y * size_z *3*sizeof(double);

static const int OFFSET_X = 0;
static const int OFFSET_Y = 1;
static const int OFFSET_Z = 2;

// Tampon générique à utiliser pour créer le fichier
char buffer_[BUFFER_SIZE];

void wait_signal()
{
    // Attend une entrée (ligne complète avec \n) sur stdin.
    std::string msg;
    std::cin >> msg;
    std::cerr << "CPP: Got signal." << std::endl;
}

void ack_signal()
{
    // Répond avec un message vide.
    std::cout << "" << std::endl;
}

double* curl_E(double *E) {
    double *curl_E = new double[size_x * size_y * size_z * 3]; // Allocate memory for the curl array

    for (size_t i = 0; i < size_x; ++i) {
        for (size_t j = 0; j < size_y; ++j) {
            for (size_t k = 0; k < size_z; ++k) {
                size_t index = (i * size_y * size_z + j * size_z + k) * 3; // Calculate index in 1D array

                if (k < size_z - 1) {
                    curl_E[index]     += E[index + 3] - E[index + 2];
                    curl_E[index + 1] -= E[index + 1 + 3] - E[index + 1];
                    curl_E[index + 2] += E[index + 0 + 3] - E[index + 0];
                }
                if (j < size_y - 1) {
                    curl_E[index]     += E[index + size_z * 3] - E[index + 2];
                    curl_E[index + 1] -= E[index + 1 + size_z * 3] - E[index + 1];
                    curl_E[index + 2] += E[index + 0 + size_z * 3] - E[index + 0];
                }
                if (i < size_x - 1) {
                    curl_E[index]     -= E[index + size_y * size_z * 3 + 1] - E[index + 1];
                    curl_E[index + 1] += E[index + size_y * size_z * 3 + 2] - E[index + 2];
                    curl_E[index + 2] -= E[index + size_y * size_z * 3 + 0] - E[index + 0];
                }
            }
        }
    }

    return curl_E;
}

double* curl_H(double *H) {
    double *curl_H = new double[size_x * size_y * size_z * 3]; // Allocate memory for the curl array

    for (size_t i = 0; i < size_x; ++i) {
        for (size_t j = 0; j < size_y; ++j) {
            for (size_t k = 0; k < size_z; ++k) {
                size_t index = (i * size_y * size_z + j * size_z + k) * 3; // Calculate index in 1D array

                if (j < size_y - 1) {
                    curl_H[index]     += H[index + size_z * 3] - H[index + 2];
                    curl_H[index + 1] -= H[index + 1 + size_z * 3] - H[index + 1];
                    curl_H[index + 2] += H[index + 0 + size_z * 3] - H[index + 0];
                }
                if (k < size_z - 1) {
                    curl_H[index]     -= H[index + 3] - H[index + 2];
                    curl_H[index + 1] += H[index + 1 + 3] - H[index + 1];
                    curl_H[index + 2] -= H[index + 0 + 3] - H[index + 0];
                }
                if (i < size_x - 1) {
                    curl_H[index]     += H[index + size_y * size_z * 3 + 1] - H[index + 1];
                    curl_H[index + 1] -= H[index + size_y * size_z * 3 + 2] - H[index + 2];
                    curl_H[index + 2] += H[index + size_y * size_z * 3 + 0] - H[index + 0];
                }
            }
        }
    }

    return curl_H;
}

void copy_matrix(double *mtx, double *mtx_to_copy, int start, int end)
{
    for (int i = start; i < end; i++)
    {
        mtx[i] = mtx_to_copy[i];
    }
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr << "CPP ERROR : no shared file provided as argv[1]" << std::endl;
        return -1;
    }
    wait_signal();

    memset(buffer_, 0, BUFFER_SIZE);
    FILE *shm_f = fopen(argv[1], "w");
    fwrite(buffer_, sizeof(char), BUFFER_SIZE, shm_f);
    fclose(shm_f);

    std::cerr << "CPP:  File ready." << std::endl;
    ack_signal();

    int shm_fd = open(argv[1], O_RDWR);
    void *shm_mmap = mmap(NULL, BUFFER_SIZE, PROT_WRITE | PROT_READ, MAP_SHARED, shm_fd, 0);
    close(shm_fd);

    if (shm_mmap == MAP_FAILED)
    {
        std::cerr << "ERROR SHM\n";
        perror(NULL);
        return -1;
    }

    double *mtx = (double *)shm_mmap;

    while (true)
    {
        wait_signal();
        double *CURL_H = curl_H(mtx);

        copy_matrix(mtx, CURL_H, 0, 3000000);

        std::cerr << "CPP: Curl H done.\n"
                  << std::endl;
        
        delete CURL_H;
        ack_signal();


        wait_signal();
        double *CURL_E = curl_E(mtx);
        copy_matrix(mtx, CURL_E, 0, 3000000);
        
        std::cerr << "CPP: Curl E done.\n"
                  << std::endl;
        delete CURL_E;
        ack_signal();
    }
    munmap(shm_mmap, BUFFER_SIZE);
    return 0;
}