addpath('Z:\Software\Libraries\commands\Matlab')

%                            57600
ser=serial('COM4','BaudRate',9600);
%setup recording for debugging
ser.RecordName='storepic-debug.txt';
ser.RecordMode='overwrite';
ser.RecordDetail='verbose';
fopen(ser);
%start recording
record(ser,'on');

asyncOpen(ser,'IMG');
command(ser,'log error');
waitReady(ser);
command(ser,'camon');
waitReady(ser);
command(ser,'resume');
waitReady(ser);
command(ser,'setimgsize');
waitReady(ser);
command(ser,'mmcinit');
waitReady(ser);
command(ser,'takepic');
waitReady(ser);
command(ser,'savepic');
line = fgetl(ser);
ser.Timeout=40;

chk = 1;
inc = 1;

while (chk > 0)
	if exist(sprintf('pic%02i.jpg',inc))
		inc = inc+1;
	else
		chk = 0;
	end
end

fname = sprintf('pic%02i.jpg',inc);

waitReady(ser,40);
imglen=sscanf(line,'Storing a %lu byte image. ');
imgout = fopen(fname,'w');

fprintf('Reading %i byte image in %i blocks\n',imglen,ceil(imglen/512));

for x=0:(ceil(imglen/512)-1)
    data = mmc_get_block(ser,x);
    disp(data);
    count=fwrite(imgout, data);
	if(count~=512)
		warning('Error writing data for block %i',x)
	end
end
fclose(imgout)
asyncClose(ser);
delete(ser)

figure;
img=imread(fname);
image(img)

