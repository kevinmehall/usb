#include <stdlib.h>
#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <sys/timeb.h>
#include <errno.h>

static struct libusb_device_handle *devh = NULL;
char inbuf[4096*32];

int find_device(void){
	devh = libusb_open_device_with_vid_pid(NULL, 0x9999, 0xffff);
	return devh ? 0 : -EIO;
}

long millis(){
	struct timeb tp;
	ftime(&tp);
	return tp.time * 1000 + tp.millitm;
}

int main(void){
	int r = 1;

	r = libusb_init(NULL);
	if (r < 0) {
		fprintf(stderr, "failed to initialise libusb\n");
		exit(1);
	}
	
	libusb_set_debug(NULL, 3);
	
	r = find_device();
	if (r < 0) {
		fprintf(stderr, "Could not find/open device\n");
		goto out;
	}

	r = libusb_claim_interface(devh, 0);
	if (r < 0) {
		fprintf(stderr, "usb_claim_interface error %d\n", r);
		goto out;
	}
	printf("claimed interface\n");
	
	int ttrans = 0, ttrans2=0;;
	long lasttime = millis();
	
	while (1){
		int transferred = 0;
		r = libusb_bulk_transfer(devh, 0x81, inbuf, 4096*32, &transferred, 100);
		ttrans += transferred;
		r = libusb_bulk_transfer(devh, 0x02, inbuf, 4096*16, &transferred, 100);
		ttrans2 += transferred;
		
		if (ttrans > 1024 * 1024){
			long t = millis();
			printf("%li %4.1f %4.1f\n", t-lasttime, ((float) ttrans)/(t-lasttime), ((float) ttrans2)/(t-lasttime));
			ttrans = 0;
			ttrans2 = 0;
			lasttime = t;
		}
	}
	
	out:
	libusb_close(devh);
	libusb_exit(NULL);
	return r >= 0 ? r : -r;
}
