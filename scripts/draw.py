#!/usr/bin/python3
import os
import sys
import datetime
import matplotlib as mpl
import ntpath
import decimal
from statistics import median
import numpy as np

mpl.use('Agg')

import matplotlib.pyplot as plt
import matplotlib.dates as md
import matplotlib
from matplotlib.widgets import Cursor
from matplotlib.widgets import MultiCursor

ctx = decimal.Context()
ctx.prec = 6

def float_to_str(t):
    f = float(t)
    d1 = ctx.create_decimal(repr(f))
    res = str(format(d1, 'f'))
    if "." in res:
       splited = res.split(".")
       res = splited[0] + (( "."+ splited[1][:9]) if splited[1][:9] != "0" else "")
    return res

def gen_pic(filenames, mode):
        values =  []
        xtsc_date =  []
        file_names = []
        full_file_names = sorted(set(filenames))
        headr = ""

        cnt = 0
        for filename in full_file_names:
                if len(filename) <= 0:
                    continue
                file_names.append(filename)
                with open(filename) as data_file:
                    for line in data_file.readlines():
                        try:
                           splited_line = line.split(' ')
                           value = float(splited_line[2])
                           values.append(value)
                           date_ts = datetime.datetime.strptime(splited_line[0], "%Y%m%d-%H:%M:%S.%f")
                           xtsc_date.append(date_ts)
                           cnt += 1
                        except:
                           pass

        fig, ax = plt.subplots()
        ax.plot(xtsc_date, values)
        
        ax.set(xlabel='time', ylabel='delta',
               title='%s entries: %s | median: %s | 99.9perc: %.2f'% (headr, len(values), median(values), float(np.percentile(values, 99.9))))
        ax.grid()
        plt.gcf().autofmt_xdate()
        
        fig.savefig("test.png")

        filename="fig"
        if len(file_names) == 1:
                filename=file_names[0]
        else:
                filename="%s/%s-%s" % (os.path.dirname(file_names[0]), ntpath.basename(file_names[0]), ntpath.basename(file_names[-1]))

        png_name = filename + ".png"
        if mode == "show":
                plt.show()
        elif mode == "all":
                plt.savefig(png_name, dpi=300, format = 'png')
                plt.show()
        elif mode == "pic":
                plt.savefig(png_name, dpi=300, format = 'png')
        else:
                print("Unknown mode")
                raise

def get_file_list():
        directory = './'
        files = os.listdir(directory)
        return filter(lambda x: x.endswith('.stat'), files)

def main():
        generate_separate_files = False
        if "-h" in sys.argv:
            print("""Help
         Usage %s [-s] <filenames>
         -h     : print help 
         """)
            sys.exit(0)

        if len(sys.argv) < 2:
                print("search files and generate *.png")
                flist = get_file_list()
                for f in flist:
                            gen_pic(f, "pic")
        else:
            filenames = sys.argv[1:]
            if generate_separate_files:
               for filename in filenames:
                   print("show diagrams and generate " + filename + ".png")
                   gen_pic([filename, ""], "pic")
            else:    
                file_name = str(sys.argv[1])
                print("show diagrams and generate " + file_name + ".png")
                gen_pic(filenames, "all")

if __name__ == "__main__":
        main()


