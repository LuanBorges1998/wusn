from PIL import Image
import glob,os

def convert_to_png(path,filename):
    img = Image.open(path)
    img.rotate(180)
    img.save(filename+".png", quality=100)


#path='/path_to_your_file'
#convert_to_png(path)

filename, file_extension = os.path.splitext('./*.ps')
os.chdir("./")
for file in glob.glob("*.ps"):
    filename, file_extension = os.path.splitext("./"+file)
    convert_to_png("./"+file,filename)