# Be careful modifying this file: line ranges from it are included in docs/jtag/as.rst.

import enum
from bitarray import bitarray



__all__ = ['ATF15xxInstr', 'ATF1502ASDevice', 'ATF1504ASDevice', 'ATF1508ASDevice']


class ATF15xxInstr(enum.IntEnum):
    EXTEST              = 0x000
    SAMPLE              = 0x055
    IDCODE              = 0x059
    ISC_READ_UES        = 0x270
    ISC_CONFIG          = 0x280
    ISC_READ            = 0x28c
    ISC_DATA            = 0x290
    ISC_PROGRAM_ERASE   = 0x29e
    ISC_ADDRESS         = 0x2a1
    ISC_LATCH_ERASE     = 0x2b3
    ISC_UNKNOWN         = 0x2bf
    BYPASS              = 0x3ff


class ATF15xxDevice:
    idcode = None # int

    fuse_count = None # int
    data_width = None # dict(range/tuple,range)

    @classmethod
    def word_size(cls, svf_row):
        for svf_rows, svf_cols in cls.data_width.items():
            if svf_row in svf_rows:
                return len(svf_cols)
        assert False

    @staticmethod
    def jed_to_svf_coords(jed_index):
        raise NotImplementedError

    @classmethod
    def jed_to_svf(cls, jed_bits):
        svf_bits = {}
        for jed_index, jed_bit in enumerate(jed_bits):
            svf_index = cls.jed_to_svf_coords(jed_index)
            if svf_index is None: continue
            svf_row, svf_col = svf_index
            if svf_row not in svf_bits:
                svf_bits[svf_row] = bitarray(cls.word_size(svf_row))
                svf_bits[svf_row].setall(1)
            svf_bits[svf_row][svf_col] = jed_bit
        return svf_bits

    @staticmethod
    def svf_to_jed_coords(svf_row, svf_col):
        raise NotImplementedError

    @classmethod
    def svf_to_jed(cls, svf_bits):
        jed_bits = bitarray(cls.fuse_count)
        jed_bits.setall(0)
        for svf_row, svf_word in svf_bits.items():
            for svf_col, svf_bit in enumerate(svf_word):
                jed_index = cls.svf_to_jed_coords(svf_row, svf_col)
                if jed_index is None: continue
                jed_bits[jed_index] = svf_bit
        return jed_bits


class ATF1502ASDevice(ATF15xxDevice):
    idcode = 0x0150203f

    fuse_count = 16808
    data_width = {
        range(  0, 108): range(86),
        range(128, 229): range(86),
        (256,): range(32),
        (512,): range(4),
        (768,): range(16),
    }

    @staticmethod
    def jed_to_svf_coords(jed_index):
        if jed_index in range(    0,  7680):
            return  12 + (jed_index -     0)  % 96, 79 - (jed_index -     0) // 96
        if jed_index in range( 7680, 15360):
            return 128 + (jed_index -  7680)  % 96, 79 - (jed_index -  7680) // 96
        if jed_index in range(15360, 16320):
            return   0 + (jed_index - 15360) // 80, 79 - (jed_index - 15360)  % 80
        if jed_index in range(16320, 16720):
            return 224 + (jed_index - 16320)  %  5, 79 - (jed_index - 16320) // 5
        if jed_index in range(16720, 16750):
            return 224 + (jed_index - 16320)  %  5, 85 - (jed_index - 16320) // 5 + 80
        if jed_index in range(16750, 16782):
            return 256, 31 - (jed_index - 16750)
        if jed_index in range(16782, 16786):
            return 512,  3 - (jed_index - 16782)
        if jed_index in range(16786, 16802):
            return 768, 15 - (jed_index - 16786)
        if jed_index in range(16802, 16808):
            return # reserved
        assert False

    @staticmethod
    def svf_to_jed_coords(svf_row, svf_col):
        if svf_row in range(  0,  12):
            if svf_col in range(0, 80):
                return 15360 + (svf_row -   0) * 80 + (79 - svf_col)
            else:
                return # always 1
        if svf_row in range( 12, 108):
            if svf_col in range(0, 80):
                return     0 + (svf_row -  12) + (79 - svf_col) * 96
            else:
                return # always 1
        if svf_row in range(128, 224):
            if svf_col in range(0, 80):
                return  7680 + (svf_row - 128) + (79 - svf_col) * 96
            else:
                return # always 1
        if svf_row in range(224, 229):
            if svf_col in range(0, 80):
                return 16320 + (svf_row - 224) + (79 - svf_col) * 5
            else:
                return 16720 + (svf_row - 224) + (85 - svf_col) * 5
        if svf_row == 256:
            return 16750 + (31 - svf_col)
        if svf_row == 512:
            return 16782 + ( 3 - svf_col)
        if svf_row == 768:
            return 16786 + (15 - svf_col)
        assert False


class ATF1504ASDevice(ATF15xxDevice):
    idcode = 0x0150403f

    fuse_count = 34192
    data_width = {
        range(  0, 108): range(166),
        range(128, 233): range(166),
        (256,): range(32),
        (512,): range(4),
        (768,): range(16),
    }

    @staticmethod
    def jed_to_svf_coords(jed_index):
        if jed_index in range(    0,   15360):
            return  12 + (jed_index -      0)  % 96,  165 - (jed_index -     0) // 96
        if jed_index in range(15360,   30720):
            return 128 + (jed_index -  15360)  % 96,  165 - (jed_index - 15360) // 96
        if jed_index in range(30720,   32640):
            return   0 + (jed_index -  30720) // 160, 165 - (jed_index - 30720)  % 160
        if jed_index in range(32640,   34134):
            return 224 + (jed_index -  32640)  %  9,  165 - (jed_index - 32640) // 9
        if jed_index in range(34134, 34166):
            return 256, 31 - (jed_index - 34134)
        if jed_index in range(34166, 34170):
            return 512,  3 - (jed_index - 34166)
        if jed_index in range(34170, 34186):
            return 768, 15 - (jed_index - 34170)
        if jed_index in range(34186, 34192):
            return # reserved
        assert False

    @staticmethod
    def svf_to_jed_coords(svf_row, svf_col):
        if svf_row in range(  0,  12):
            if svf_col in range(6, 166):
                return 30720 + (svf_row -   0) * 160 + (165 - svf_col)
            else:
                return # always 1
        if svf_row in range( 12, 108):
            if svf_col in range(6, 166):
                return     0 + (svf_row -  12) + (165 - svf_col) * 96
            else:
                return # always 1
        if svf_row in range(128, 224):
            if svf_col in range(6, 166):
                return 15360 + (svf_row - 128) + (165 - svf_col) * 96
            else:
                return # always 1
        if svf_row in range(224, 233):
            if svf_col in range(0, 166):
                return 32640 + (svf_row - 224) + (165 - svf_col) * 9
            else:
                return # always 1
        if svf_row == 256:
            return 34134 + (31 - svf_col)
        if svf_row == 512:
            return 34166 + ( 3 - svf_col)
        if svf_row == 768:
            return 34170 + (15 - svf_col)
        assert False


class ATF1508ASDevice(ATF15xxDevice):
    idcode = 0x0150803f

    fuse_count = 74136
    data_width = {
        range(  0, 108): range(326),
        range(128, 251): range(326),
        (256,): range(32),
        (512,): range(4),
        (768,): range(16),
    }

    @staticmethod
    def jed_to_svf_coords(jed_index):
        if jed_index in range(    0, 30720):
            return  12 + (jed_index -     0)  %  96, 325 - (jed_index -     0) // 96
        if jed_index in range(30720, 61440):
            return 128 + (jed_index - 30720)  %  96, 325 - (jed_index - 30720) // 96
        if jed_index in range(61440, 65280):
            return   0 + (jed_index - 61440) // 320, 325 - (jed_index - 61440)  % 320
        if jed_index in range(65280, 74082):
            return 224 + (jed_index - 65280)  %  27, 325 - (jed_index - 65280) // 27
        if jed_index in range(74082, 74114):
            return 256, 31 - (jed_index - 74082)
        if jed_index in range(74114, 74118):
            return 512,  3 - (jed_index - 74114)
        if jed_index in range(74118, 74134):
            return 768, 15 - (jed_index - 74118)
        if jed_index in range(74134, 74136):
            return # reserved
        assert False

    @staticmethod
    def svf_to_jed_coords(svf_row, svf_col):
        if svf_row in range(  0,  12):
            if svf_col in range(6, 326):
                return 61440 + (svf_row -   0) * 320 + (325 - svf_col)
            else:
                return # always 1
        if svf_row in range( 12, 108):
            if svf_col in range(6, 326):
                return     0 + (svf_row -  12) + (325 - svf_col) * 96
            else:
                return # always 1
        if svf_row in range(128, 224):
            if svf_col in range(6, 326):
                return 30720 + (svf_row - 128) + (325 - svf_col) * 96
            else:
                return # always 1
        if svf_row in range(224, 251):
            if svf_col in range(0, 326):
                return 65280 + (svf_row - 224) + (325 - svf_col) * 27
            else:
                return # always 1
        if svf_row == 256:
            return 74082 + (31 - svf_col)
        if svf_row == 512:
            return 74114 + ( 3 - svf_col)
        if svf_row == 768:
            return 74118 + (15 - svf_col)
        assert False


if __name__ == '__main__':
    with open('atf1502as_svf2jed.csv', 'w') as f:
        f.write('SVF ROW,SVF COL,JED\n')
        for svf_rows, svf_cols in ATF1502ASDevice.data_width.items():
            for svf_row in svf_rows:
                for svf_col in svf_cols:
                    jed_index = ATF1502ASDevice.svf_to_jed_coords(svf_row, svf_col)
                    if jed_index is None: jed_index = 0x7fff
                    f.write('{},{},{}\n'.format(svf_row, svf_col, jed_index))

    with open('atf1502as_jed2svf.csv', 'w') as f:
        f.write('JED,SVF ROW,SVF COL\n')
        for jed_index in range(ATF1502ASDevice.fuse_count):
            svf_index = ATF1502ASDevice.jed_to_svf_coords(jed_index)
            if svf_index is None: continue
            f.write('{},{},{}\n'.format(jed_index, *svf_index))

    with open('atf1504as_svf2jed.csv', 'w') as f:
        f.write('SVF ROW,SVF COL,JED\n')
        for svf_rows, svf_cols in ATF1504ASDevice.data_width.items():
            for svf_row in svf_rows:
                for svf_col in svf_cols:
                    jed_index = ATF1504ASDevice.svf_to_jed_coords(svf_row, svf_col)
                    if jed_index is None: jed_index = 0xffff
                    f.write('{},{},{}\n'.format(svf_row, svf_col, jed_index))

    with open('atf1504as_jed2svf.csv', 'w') as f:
        f.write('JED,SVF ROW,SVF COL\n')
        for jed_index in range(ATF1504ASDevice.fuse_count):
            svf_index = ATF1504ASDevice.jed_to_svf_coords(jed_index)
            if svf_index is None: continue
            f.write('{},{},{}\n'.format(jed_index, *svf_index))

    with open('atf1508as_svf2jed.csv', 'w') as f:
        f.write('SVF ROW,SVF COL,JED\n')
        for svf_rows, svf_cols in ATF1508ASDevice.data_width.items():
            for svf_row in svf_rows:
                for svf_col in svf_cols:
                    jed_index = ATF1508ASDevice.svf_to_jed_coords(svf_row, svf_col)
                    if jed_index is None: jed_index = 0x1ffff
                    f.write('{},{},{}\n'.format(svf_row, svf_col, jed_index))

    with open('atf1508as_jed2svf.csv', 'w') as f:
        f.write('JED,SVF ROW,SVF COL\n')
        for jed_index in range(ATF1508ASDevice.fuse_count):
            svf_index = ATF1508ASDevice.jed_to_svf_coords(jed_index)
            if svf_index is None: continue
            f.write('{},{},{}\n'.format(jed_index, *svf_index))
