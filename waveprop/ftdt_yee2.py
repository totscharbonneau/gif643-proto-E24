# GPL3, Copyright (c) Max Hofheinz, UdeS, 2021
import numpy
import fiddle
import mmap
from subprocess import Popen, PIPE

PNAME = "../ftdt_yee"
FNAME = "GIF642-problematique-shm"
MATRIX_SIZE = 100


def curl_E(E):
    curl_E = numpy.zeros(E.shape)
    curl_E[:, :-1, :, 0] += E[:, 1:, :, 2] - E[:, :-1, :, 2]
    curl_E[:, :, :-1, 0] -= E[:, :, 1:, 1] - E[:, :, :-1, 1]

    curl_E[:, :, :-1, 1] += E[:, :, 1:, 0] - E[:, :, :-1, 0]
    curl_E[:-1, :, :, 1] -= E[1:, :, :, 2] - E[:-1, :, :, 2]

    curl_E[:-1, :, :, 2] += E[1:, :, :, 1] - E[:-1, :, :, 1]
    curl_E[:, :-1, :, 2] -= E[:, 1:, :, 0] - E[:, :-1, :, 0]
    return curl_E


def curl_H(H):
    curl_H = numpy.zeros(H.shape)
    curl_H[:, 1:, :, 0] += H[:, 1:, :, 2] - H[:, :-1, :, 2]
    curl_H[:, :, 1:, 0] -= H[:, :, 1:, 1] - H[:, :, :-1, 1]

    curl_H[:, :, 1:, 1] += H[:, :, 1:, 0] - H[:, :, :-1, 0]
    curl_H[1:, :, :, 1] -= H[1:, :, :, 2] - H[:-1, :, :, 2]

    curl_H[1:, :, :, 2] += H[1:, :, :, 1] - H[:-1, :, :, 1]
    curl_H[:, 1:, :, 2] -= H[:, 1:, :, 0] - H[:, :-1, :, 0]
    return curl_H


def timestep(E, H, courant_number, source_pos, source_val):
    curl_h = curl_H(H)
    E += courant_number * curl_h
    E[source_pos] += source_val
    H -= courant_number * curl_E(E)
    return E, H


def curl_subprocess():
    subproc = Popen([PNAME, FNAME], stdin=PIPE, stdout=PIPE)
    return subproc


def signal_and_wait(subproc):
    subproc.stdin.write("S\n".encode())
    subproc.stdin.flush()
    subproc.stdout.readline()
    


class WaveEquation:
    def __init__(self, s, courant_number, source):
        self.curl_proc = curl_subprocess()
        signal_and_wait(self.curl_proc)
        self.shm_f = open(FNAME, "r+b")
        self.shm_mm = mmap.mmap(self.shm_f.fileno(), 0)
        self.shared_matrix = numpy.ndarray(
            shape=(MATRIX_SIZE, MATRIX_SIZE, MATRIX_SIZE, 3),
            dtype=numpy.float64,
            buffer=self.shm_mm,
        )

        s = s + (3,)

        self.E = numpy.zeros(s)
        self.H = numpy.zeros(s)
        self.courant_number = courant_number
        self.source = source
        self.index = 0

    def __call__(self, figure, field_component, slice, slice_index, initial=False):
        if field_component < 3:
            field = self.E
        else:
            field = self.H
            field_component = field_component % 3
        if slice == 0:
            field = field[slice_index, :, :, field_component]
        elif slice == 1:
            field = field[:, slice_index, :, field_component]
        elif slice == 2:
            field = field[:, :, slice_index, field_component]
        source_pos, source_index = source(self.index)

        self.timestep(source_pos, source_index)
        #self.E, self.H = timestep(self.E, self.H, self.courant_number, source_pos, source_index)

        if initial:
            axes = figure.add_subplot(111)
            self.image = axes.imshow(field, vmin=-1e-2, vmax=1e-2)
        else:
            self.image.set_data(field)
        self.index += 1

    def timestep(self, source_pos, source_val):
        signal_and_wait(self.curl_proc)
        self.E += self.courant_number * self.shared_matrix
        self.E[source_pos] += source_val

        self.shared_matrix[:, :, :, :] = self.E[:, :, :, :]

        signal_and_wait(self.curl_proc)
        self.H -= self.courant_number * self.shared_matrix
        self.shared_matrix[:, :, :, :] = self.H[:, :, :, :]
        print(f"E : {numpy.sum(self.E)}")
        print(f"H : {numpy.sum(self.H)}")


if __name__ == "__main__":
    n = 100
    r = 0.01
    l = 30

    def source(index):
        return ([n // 3], [n // 3], [n // 2], [0]), 0.1 * numpy.sin(0.1 * index)

    w = WaveEquation((n, n, n), 0.1, source)
    fiddle.fiddle(
        w,
        [
            ("field component", {"Ex": 0, "Ey": 1, "Ez": 2, "Hx": 3, "Hy": 4, "Hz": 5}),
            ("slice", {"XY": 2, "YZ": 0, "XZ": 1}),
            ("slice index", 0, n - 1, n // 2, 1),
        ],
        update_interval=0.01,
    )