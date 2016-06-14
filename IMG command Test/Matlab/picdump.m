function [data]=picdump(com,baud)
    if(~exist('baud','var') || isempty(baud))
        baud=57600;
    end
    if ~exist('com','var') || isempty(com)
        com='COM5';
    end
    
    %constants for imager
    BT_IMG_START=uint16(sscanf('0x990F','0x%X'));
    BT_IMG_BODY=uint16(sscanf('0x99F0','0x%X'));
    %TODO: read from imager
    IMG_SLOT_SIZE=150;
    NUM_IMG_SLOTS=5;
    IMG_ADDR_START=64+10;
    
    
    try
        oldp=addpath('Z:\Software\Libraries\commands\Matlab');

        ser=serial(com,'BaudRate',baud);
        %setup recording for debugging
        %ser.RecordName='storepic-debug.txt';
        %ser.RecordMode='overwrite';
        %ser.RecordDetail='verbose';
        fopen(ser);
        %start recording
        %record(ser,'on');
        
        fprintf('Connecting to imager\n');
        
        for k=1:NUM_IMG_SLOTS
            
            img_start=IMG_ADDR_START+(k-1)*IMG_SLOT_SIZE;
            fprintf('Transfering First Image Block from image slot #%i\n',k-1);
            %get first block of image
            block = mmc_get_block(ser,img_start);

            %check magic
            if(typecast(block(1:2),'uint16')~=BT_IMG_START)
                fprintf('Incorrect Start block header, skipping image\n');
                continue;
            end

            %get image number
            num=double(block(3));
            %get number of blocks
            blocks=double(block(4));

            fprintf('Reading image %i in %i blocks\n',num,blocks);
            time=typecast(block(5:8),'uint32');
            fprintf('Image taken at time %u\n',time);

            %allocate array for image data
            data=zeros(1,506*blocks,'uint8');
            %store data in array
            data(1:506)=block(5:510);

            for blk=1:(blocks-1)
                %read data and put it in the array
                block = mmc_get_block(ser,img_start+blk);
                %check magic
                if(typecast(block(1:2),'uint16')~=BT_IMG_BODY)
                    error('Incorrect block header');
                end
                %get image number
                blkInum=double(block(3));
                %check to see if image numbers match
                if(num~=blkInum)
                    error('Incorrect Image Number');
                end
                %get block number
                blknum=double(block(4));
                %TODO: check CRC
                %store data in the array
                data((1:506)+506*blknum)=block(5:510);
            end

            %find the first usable file name
            chk = 1;inc = 1;
            while (chk > 0)
                fname = sprintf('pic%02i.jpg',inc);
                if exist(fname,'file')
                    inc = inc+1;
                else
                    chk = 0;
                end
            end

            %open image file
            imgout = fopen(fname,'w');
            %write data to file skip time
            count=fwrite(imgout, data(5:end));
            %close file
            fclose(imgout);
            if(count~=length(data(5:end)))
                error('Failed to write image data to file');
            else
                fprintf('Image file "%s" written\n',fname);
            end
        end
    catch err
        %close file if it is open
        if(exist('imgout','var') && any(fopen('all')==imgout))
            fclose(imgout);
        end
        
        if exist('ser','var')
            if strcmp(ser.Status,'open')
                %wait for everything to finish
                while ser.BytesToOutput
                end
                record(ser,'off');
                fclose(ser);
            end
            delete(ser);
        end
        path(oldp);
        
        rethrow(err)
    end 
    delete(ser); 
    
    %display picture
    figure;
    img=imread(fname);
    img=imrotate(img,-90);
    imshow(img);
end

