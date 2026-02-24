import imageio.v2 as iio
import numpy as np
import sys
import matplotlib.pyplot as plt

img_size = 26
hex_mask_bin = '1111111100000000'
hex_mask = int(hex_mask_bin,2)
byte_dat = ["0"] * 8

if __name__ == '__main__':
    file_name = sys.argv[1]
    array_name = sys.argv[2]
    file_path = file_name[:-4] # Remove .png
    img = iio.imread(file_name)
    rows = len(img)
    columns = len(img[0])
    try:
        f = open(f'{file_path}-converted.txt','w')
    except:
        print("An error has occured, output file could not be created\n")
        quit()
        
        
    f.write(f'static const uint8_t {array_name}[] = ' + '{\n')
    f.write(f'0x{(rows & hex_mask) >> 8:02X},0x{(rows & ~hex_mask):02X},0x{(columns & hex_mask) >> 8:02X},0x{(columns & ~hex_mask):02X},\n')
    print(img)
    k = 0
    g = -1
    for i, pixel in enumerate(img):
        pixel_reversed = pixel[::-1]
        for j, pixel_dat in enumerate(pixel_reversed):
            R_val = pixel_dat[0]
            byte_val = "1" if R_val > 120 else "0"
            byte_dat[k%8] = byte_val


            if(k%8 == 0 and i != 0):
                g += 1
                g %= 8
                print(byte_dat)
                byte_str = "".join(byte_dat)
                f.write(f'0x{int(byte_str,2):02X}')
                byte_dat = ["0"] * 8
                if(not (i == rows - 1 and j == columns - 1)):
                    f.write(',')
                if(g == 7):
                    f.write('\n')
            k += 1
        string_status = f'Converting to {file_path}-converted.txt: '
        for h in range(20):
            string_status += "-" if h / 20 > i / rows else "X"
        print(string_status,end='\n')
    f.write('};')
    
    f.close()
    print("Done!")