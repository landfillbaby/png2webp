#!/usr/bin/env python
# anti-copyright Lucy Phipps 2023
if __name__ == '__main__':
    from argparse import ArgumentParser as f
    from struct import Struct as p
    f = f()
    f, t = f.add_argument, f.parse_args
    f('exe', help='Windows PE32(+) file')
    def int_0(x): return int(x, 0)
    f('stamp', type=int_0, nargs='?', help='new Unix timestamp,'
      ' decimal, octal (leading 0o), or hexadecimal (leading 0x)')
    f, p = t(), p('<I')
    f, t, p, u = f.exe, f.stamp, p.pack, p.unpack
    with open(f, 'rb' if t is None else 'rb+') as f:
        r, w, s = f.read, f.write, f.seek
        def f(): return u(r(4))[0]
        m = 'Not a Windows PE32(+) file'
        if r(2) != b'MZ': raise ValueError(m)
        s(60)
        s(f())
        if r(4) != b'PE\0\0': raise ValueError(m)
        s(4, 1)
        if t is None: print(f())
        else:
            t &= 0xffffffff
            print('old:', f())
            print('new:', t)
            s(-4, 1)
            w(p(t))
