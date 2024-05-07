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
static const int BUFFER_SIZE = size_x * size_y * size_z *3*sizeof(double);

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

    // Compute curl_E
    for (int i = 0; i < size_x; ++i) {
        for (int j = 0; j < size_y - 1; ++j) {
            for (int k = 0; k < size_z; ++k) {
                curl_E[i * size_y * size_z * 3 + j * size_z * 3 + k * 3] += E[i * size_y * size_z * 3 + (j + 1) * size_z * 3 + k * 3 + 2] - E[i * size_y * size_z * 3 + j * size_z * 3 + k * 3 + 2];
            }
        }
    }

    for (int i = 0; i < size_x; ++i) {
        for (int j = 0; j < size_y; ++j) {
            for (int k = 0; k < size_z - 1; ++k) {
                curl_E[i * size_y * size_z * 3 + j * size_z * 3 + k * 3] -= E[i * size_y * size_z * 3 + j * size_z * 3 + (k + 1) * 3 + 1] - E[i * size_y * size_z * 3 + j * size_z * 3 + k * 3 + 1];
            }
        }
    }

    for (int i = 0; i < size_x; ++i) {
        for (int j = 0; j < size_y; ++j) {
            for (int k = 0; k < size_z - 1; ++k) {
                curl_E[i * size_y * size_z * 3 + j * size_z * 3 + k * 3 + 1] += E[i * size_y * size_z * 3 + j * size_z * 3 + (k + 1) * 3] - E[i * size_y * size_z * 3 + j * size_z * 3 + k * 3];
            }
        }
    }

    for (int i = 0; i < size_x - 1; ++i) {
        for (int j = 0; j < size_y; ++j) {
            for (int k = 0; k < size_z; ++k) {
                curl_E[i * size_y * size_z * 3 + j * size_z * 3 + k * 3 + 1] -= E[(i + 1) * size_y * size_z * 3 + j * size_z * 3 + k * 3 + 2] - E[i * size_y * size_z * 3 + j * size_z * 3 + k * 3 + 2];
            }
        }
    }

    for (int i = 0; i < size_x - 1; ++i) {
        for (int j = 0; j < size_y; ++j) {
            for (int k = 0; k < size_z; ++k) {
                curl_E[i * size_y * size_z * 3 + j * size_z * 3 + k * 3 + 2] += E[(i + 1) * size_y * size_z * 3 + j * size_z * 3 + k * 3 + 1] - E[i * size_y * size_z * 3 + j * size_z * 3 + k * 3 + 1];
            }
        }
    }

    for (int i = 0; i < size_x; ++i) {
        for (int j = 0; j < size_y - 1; ++j) {
            for (int k = 0; k < size_z; ++k) {
                curl_E[i * size_y * size_z * 3 + j * size_z * 3 + k * 3 + 2] -= E[i * size_y * size_z * 3 + (j + 1) * size_z * 3 + k * 3] - E[i * size_y * size_z * 3 + j * size_z * 3 + k * 3];
            }
        }
    }
    return curl_E;
}

double* curl_H(double *H) {
    double *curl_H = new double[size_x * size_y * size_z * 3]; // Allocate memory for the curl array

    // Compute curl_H
    for (int i = 0; i < size_x; ++i) {
        for (int j = 1; j < size_y; ++j) {
            for (int k = 0; k < size_z; ++k) {
                curl_H[i * size_y * size_z * 3 + j * size_z * 3 + k * 3] += H[i * size_y * size_z * 3 + j * size_z * 3 + k * 3 + 2] - H[i * size_y * size_z * 3 + (j - 1) * size_z * 3 + k * 3 + 2];
            }
        }
    }
    for (int i = 0; i < size_x; ++i) {
        for (int j = 0; j < size_y; ++j) {
            for (int k = 1; k < size_z; ++k) {
                curl_H[i * size_y * size_z * 3 + j * size_z * 3 + k * 3] -= H[i * size_y * size_z * 3 + j * size_z * 3 + k * 3 + 1] - H[i * size_y * size_z * 3 + j * size_z * 3 + (k - 1) * 3 + 1];
            }
        }
    }

    for (int i = 0; i < size_x; ++i) {
        for (int j = 0; j < size_y; ++j) {
            for (int k = 1; k < size_z; ++k) {
                curl_H[i * size_y * size_z * 3 + j * size_z * 3 + k * 3 + 1] += H[i * size_y * size_z * 3 + j * size_z * 3 + k * 3] - H[i * size_y * size_z * 3 + j * size_z * 3 + (k - 1) * 3];
            }
        }
    }

    for (int i = 1; i < size_x; ++i) {
        for (int j = 0; j < size_y; ++j) {
            for (int k = 0; k < size_z; ++k) {
                curl_H[i * size_y * size_z * 3 + j * size_z * 3 + k * 3 + 1] -= H[i * size_y * size_z * 3 + j * size_z * 3 + k * 3 + 2] - H[(i - 1) * size_y * size_z * 3 + j * size_z * 3 + k * 3 + 2];
            }
        }
    }

    for (int i = 1; i < size_x; ++i) {
        for (int j = 0; j < size_y; ++j) {
            for (int k = 0; k < size_z; ++k) {
                curl_H[i * size_y * size_z * 3 + j * size_z * 3 + k * 3 + 2] += H[i * size_y * size_z * 3 + j * size_z * 3 + k * 3 + 1] - H[(i - 1) * size_y * size_z * 3 + j * size_z * 3 + k * 3 + 1];
            }
        }
    }

    for (int i = 0; i < size_x; ++i) {
        for (int j = 1; j < size_y; ++j) {
            for (int k = 0; k < size_z; ++k) {
                curl_H[i * size_y * size_z * 3 + j * size_z * 3 + k * 3 + 2] -= H[i * size_y * size_z * 3 + (j - 1) * size_z * 3 + k * 3] - H[i * size_y * size_z * 3 + j * size_z * 3 + k * 3];
            }
        }
    }

    return curl_H;
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
        std::cerr << "CPP: Curl H done.\n"
                  << std::endl;
        
        delete CURL_H;
        ack_signal();

        wait_signal();
        double *CURL_E = curl_E(mtx);
        std::cerr << "CPP: Curl E done.\n"
                  << std::endl;
        delete CURL_E;
        ack_signal();
    }
    munmap(shm_mmap, BUFFER_SIZE);
    return 0;
}