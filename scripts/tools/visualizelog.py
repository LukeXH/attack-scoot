
import argparse
import csv
from matplotlib import pyplot as plt
import numpy as np

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--im-log-csv-file", help="file for image as a csv")
    args = parser.parse_args()

    with open(args.im_log_csv_file,newline='\n') as csvfile:
        im_arr = np.genfromtxt(csvfile, dtype=int, delimiter=',')
        im_arr = im_arr[:,:-1]
        im_arr_4d = im_arr.reshape(im_arr.shape[0], int(im_arr.shape[1]/4),4)
        print(im_arr)
        print(im_arr.shape)

        plt.imshow(im_arr_4d)
        plt.show()

        # log_reader = csv.reader(csvfile, delimiter=',')
        # for row in log_reader:
        #     print(', '.join(row))