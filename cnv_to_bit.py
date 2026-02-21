import imageio.v2 as iio
import numpy as np
import sys
import matplotlib.pyplot as plt

img_size = 26
hex_mask_bin = '1111111100000000'
hex_mask = int(hex_mask_bin,2)

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
        
        
    f.write(f'static const uint8_t {array_name}[] = ' + '{')
    f.write(f'0x{(rows & hex_mask) >> 8:02X},0x{(rows & ~hex_mask):02X},0x{(columns & hex_mask) >> 8:02X},0x{(columns & ~hex_mask):02X},\n')
    print(img)
    for i, pixel in enumerate(img):
        pixel_reversed = pixel[::-1]
        for j, pixel_dat in enumerate(pixel_reversed):
            R_val = ((int)(pixel_dat[0] * 63) // 255) << 2
            G_val = ((int)(pixel_dat[1] * 63) // 255) << 2
            B_val = ((int)(pixel_dat[2] * 63) // 255) << 2
            #data += f'0x{R_val:02X},0x{G_val:02X},0x{B_val:02X},'
            f.write(f'0x{B_val:02X},0x{G_val:02X},0x{R_val:02X}')
            if(not (i == rows - 1 and j == columns - 1)):
                f.write(',')
            if((j+1+i*rows) % 3 == 0 and not (i == rows - 1 and j == columns - 1)):
                f.write('\n')
        string_status = f'Converting to {file_path}-converted.txt: '
        for h in range(20):
            string_status += "-" if h / 20 > i / rows else "X"
        print(string_status,end='\n')
    f.write('};')
    
    f.close()
    print("Done!")