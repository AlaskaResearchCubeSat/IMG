function Storepic(com,baud)
    if(~exist('baud','var') || isempty(baud))
        baud=57600;
    end
    if ~exist('com','var') || isempty(com)
        com='COM5';
    end
    
    try
        oldp=addpath('Z:\Software\Libraries\commands\Matlab');

        ser=serial(com,'BaudRate',baud);
        %setup recording for debugging
        %ser.RecordName='storepic-debug.txt';
        %ser.RecordMode='overwrite';
        %ser.RecordDetail='verbose';
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
            if exist(sprintf('pic%02i.jpg',inc),'file')
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
            count=fwrite(imgout, data);
            if(count~=512)
                warning('Error writing data for block %i',x)
            end
        end
    catch err
        path(oldp);
        
        if(exist('imgout','var'))
            fclose(imgout);
        end
        
        if exist('ser','var')
            if strcmp(ser.Status,'open')
                %close async connection
                asyncClose(ser);
                %wait for everything to finish
                while ser.BytesToOutput
                end
                record(ser,'off');
                fclose(ser);
            end
            delete(ser);
        end
        
        rethrow(err)
    end 
    %cleanup
    fclose(imgout);
    asyncClose(ser);
    delete(ser); 
    
    %display picture
    figure;
    img=imread(fname);
    image(img)
end

