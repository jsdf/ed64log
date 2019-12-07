/* loader64.c
   libftdi Everdrive 64 USB-tool
   by saturnu
*/

#include <stdio.h>
#ifdef __APPLE__
#include <libftdi1/ftdi.h>
#else
#include <ftdi.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "gopt.h"
#include "loader64.h"

int main( int argc, const char **argv )
{
	
	//usb buffer
	char send_buff[512];
    char recv_buff[512];    
	
	int arg_fail=1;
	
	//rom filename
	const char *filename;
	
	//verbosity level 0-2
	int verbosity;
	int i,print_help;  
	
	void *options= gopt_sort( & argc, argv, gopt_start(
	gopt_option( 'h', 0, gopt_shorts( 'h' ), gopt_longs( "help" )),
	gopt_option( 'z', 0, gopt_shorts( 'z' ), gopt_longs( "version" )),
	gopt_option( 'v', GOPT_REPEAT, gopt_shorts( 'v' ), gopt_longs( "verbose" )),
	gopt_option( 'r', 0, gopt_shorts( 'r' ), gopt_longs( "read" )),	
	gopt_option( 'w', 0, gopt_shorts( 'w' ), gopt_longs( "write" )),		
	gopt_option( 'p', 0, gopt_shorts( 'p' ), gopt_longs( "pifboot" )),
	gopt_option( 'f', GOPT_ARG, gopt_shorts( 'f' ), gopt_longs( "file" ))));	
	
	

  if( gopt( options, 'h' ) ){
    fprintf( stdout, "Syntax: sudo ./loader64 [options] ...\n\n");
    fprintf( stdout, "loader64 - Everdrive64 USB-tool\n" );
    fprintf( stdout, "by saturnu <tt@anpa.nl>\n\n" );
    fprintf( stdout, " -h, --help\t\tdisplay this help and exit\n" );    
    fprintf( stdout, " -v, --verbose\t\tverbose\n" );
    fprintf( stdout, " -f, --file=rom.z64\trom in z64 format\n" );
    //TODO:
    //fprintf( stdout, " -r, --read\t\tread from sdram\n" );
    fprintf( stdout, " -w, --write\t\twrite to sdram\n" );    
    fprintf( stdout, " -p, --pifboot\t\tsimulate pifboot CIC-6102\n" );       
    fprintf( stdout, " -z, --version\t\tversion\n" );    
               
    exit( EXIT_SUCCESS );
  }


  //show info without options
  print_help=1;

  if( gopt( options, 'p' ) || gopt( options, 'w' ) || gopt( options, 'r') || gopt( options, 'z') || gopt( options, 'h') || gopt( options, 'v') ){	

		print_help = 0;	
		 if( gopt( options, 'p' ) || gopt( options, 'w' ) )
			arg_fail = 0;
  }
  
  
  if( gopt( options, 'w' ) && gopt( options, 'r' )){
	
    fprintf( stdout, "error: could not read and write at the same time\n" );
    exit( EXIT_SUCCESS );
  }
  
    if( gopt( options, 'w' ) && gopt( options, 'p' )){
	
    fprintf( stdout, "error: use pifboot separately\n" );
    exit( EXIT_SUCCESS );
  }


  if( gopt( options, 'z' ) ){
	
    fprintf( stdout, "loader64 version v%d.%d\n", MAJOR_VERSION, MINOR_VERSION );
    exit( EXIT_SUCCESS );
  }


  verbosity = gopt( options, 'v' );


  if( verbosity > 1 )
    fprintf( stderr, "being really verbose\n" );

  else if( verbosity )
    fprintf( stderr, "being verbose\n" );


//options are ok - init ftdi
if(!arg_fail){

    int ret;
    struct ftdi_context *ftdi;
    if ((ftdi = ftdi_new()) == 0)
   {
        fprintf(stderr, "ftdi_new failed\n");
        return EXIT_FAILURE;
    }
 
    if ((ret = ftdi_usb_open(ftdi, USB_VENDOR, USB_DEVICE)) < 0)
    {
        fprintf(stderr, "unable to open ftdi device: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
        ftdi_free(ftdi);
        return EXIT_FAILURE;
    }else{
		//read/write timeout e.g. 500ms
	    ftdi->usb_read_timeout = USB_READ_TIMEOUT;
		ftdi->usb_write_timeout = USB_WRITE_TIMEOUT;
	}

    if (ftdi->type == TYPE_R)
    {
        unsigned int chipid;
        if(verbosity >= 1){
			printf("ftdi_read_chipid: %d\n", ftdi_read_chipid(ftdi, &chipid));
			printf("FTDI chipid: %X\n", chipid);
		}
    }
    
    
    //init usb transfer buffer
    memset(send_buff, 0, 512);
    memset(recv_buff, 0, 512);    
    
    send_buff[0]='C';
    send_buff[1]='M';
    send_buff[2]='D';
    send_buff[3]='T'; //test        
    
    int ret_s = ftdi_write_data(ftdi, send_buff, 512);
    
    if(verbosity >= 1)    
    printf("send: %i bytes\n",ret_s);
    
    int ret_r = ftdi_read_data(ftdi, recv_buff, 512);
    
    if(verbosity >= 1)    
	printf("recv: %i bytes\n",ret_r);
	
	if(recv_buff[3]=='k'){
	 printf("init test: ok\n");
	}
	else{
		printf("init test: faild - ED64 not running?\n");
		ftdi_free(ftdi);
		exit( EXIT_SUCCESS );
	}


    memset(send_buff, 0, 512);
    memset(recv_buff, 0, 512);   
	ret_s = 0;
	ret_r = 0;

	//pifboot
	if( gopt( options, 'p' ) ){
		send_buff[0]='C';
		send_buff[1]='M';
		send_buff[2]='D';
		send_buff[3]='S'; //pif boot
			
		ret_s = ftdi_write_data(ftdi, send_buff, 512);
		
		if(verbosity >= 1)
		printf("send: %i bytes\n",ret_s);
		printf("pif simulation instructed...\n");
		
	}else //write
	if( gopt( options, 'w' ) ){	
		

		 if( gopt_arg( options, 'f', & filename ) && strcmp( filename, "-" ) ){	
			 
			FILE *fp;
			fp=fopen(filename, "rb");
			int fsize;

			struct stat st;
			stat(filename, &st);
			fsize = st.st_size;

			if(verbosity >= 1)
			printf("test_size: %d\n",fsize);


			int length = (int) fsize;
			
			if(verbosity >= 1)			
			printf("length: %d\n",length);
			
			if (((length / 0x10000) * 0x10000) != fsize) {
				length = (int) (((fsize / 0x10000) * 0x10000) + 0x10000);
			}


			if(verbosity >= 2)			
			printf("length_buffer: %d\n",length);

			//FILE_CHUNK default 0x8000
			char buffer[FILE_CHUNK];
			memset(buffer, 0, FILE_CHUNK);
			
			if(verbosity >= 2)			
			printf("buffer created\n");
			
			
			//tiny rom fill-mode
		     if (length < 0x200000){
				 
				if(verbosity >= 1)
				printf("needs filling\n");
				
				send_buff[0]='C';
				send_buff[1]='M';
				send_buff[2]='D';
				send_buff[3]='F'; //fill

				ret_s = ftdi_write_data(ftdi, send_buff, 512);

				if(verbosity >= 1)
					printf("send: %i bytes\n",ret_s);

				sleep(1);
				int ret_r = ftdi_read_data(ftdi, recv_buff, 512);

				if(verbosity >= 1)    
					printf("recv: %i bytes\n",ret_r);

				if(recv_buff[3]=='k'){
					printf("fill test: ok\n");
					memset(send_buff, 0, 512);
					memset(recv_buff, 0, 512);   
					ret_s = 0;
					ret_r = 0;

				}else{
					printf("fill test: error\n");
					ftdi_free(ftdi);
					exit( EXIT_SUCCESS );				
				}
			 }
			
			
			send_buff[0]='C';
			send_buff[1]='M';
			send_buff[2]='D'; 
			send_buff[3]='W'; //write
			send_buff[4]=0; //offset
			send_buff[5]=0; //offset		
			send_buff[6] = (char) ((( length) / 0x200) >> 8); //length
			send_buff[7] = (char) (length / 0x200); //length

			ret_s = ftdi_write_data(ftdi, send_buff, 512);

			if(verbosity >= 1)
				printf("send write cmd: %i bytes\n",ret_s);

			//now write in 0x8000 chunks -> 32768

			printf("sending...\n");
			int s;
			for(s = 0;s < length; s += 0x8000){
			
				if (s == 0x2000000) //step 1024
				{
					memset(send_buff, 0, 512);
					memset(recv_buff, 0, 512);   
					ret_s = 0;
					ret_r = 0;
					
					send_buff[0]='C';
					send_buff[1]='M';
					send_buff[2]='D'; 
					send_buff[3]='W'; //write
					send_buff[4]=0x40; //32mb offset
					send_buff[5]=0; //offset
					send_buff[6] = (char) (( ( length - 0x2000000) / 0x200) >> 8); //length
					send_buff[7] = (char) (( length - 0x2000000) / 0x200); //length

					ret_s = ftdi_write_data(ftdi, send_buff, 512);

					if(verbosity >= 1)
						printf("send offset cmd: %i bytes\n",ret_s);
				}
			
			//read parts to memory
			int fret = fread(buffer, sizeof(buffer[0]), sizeof(buffer)/sizeof(buffer[0]), fp);
			
			ret_s = ftdi_write_data(ftdi, buffer,  0x8000);
			
				if ((s % 0x80000) == 0)
				{
					if(verbosity >= 1 && s!=0)
					printf("part sent: %i\n",s);
				}
			}
			
			printf("upload done...\n",  argv[0]);
        
		}		
		
		
	}else //TODO: read
	if( gopt( options, 'r' ) ){	
		send_buff[0]='C';
		send_buff[1]='M';
		send_buff[2]='D';
		send_buff[3]='R'; //read
	}
    
    
    
    if ((ret = ftdi_usb_close(ftdi)) < 0)
    {
        fprintf(stderr, "unable to close ftdi device: %d (%s)\n", ret, ftdi_get_error_string(ftdi));
        ftdi_free(ftdi);
        return EXIT_FAILURE;
    }
    
    
    ftdi_free(ftdi);
    
    
}
    
    if(print_help){

		printf("%s: missing operand\n",  argv[0]);
		printf("Try '%s --help' for more information.\n",  argv[0]);
	}
    
    
	gopt_free( options );
    return EXIT_SUCCESS;
}


