/* File Interface 
 *  (c) Jack Lange, 2012
 */

#include <linux/fcntl.h>

int file_mkdir(const char * pathname, unsigned short perms, int recurse);


/* MODE: 
   O_RDWR
   O_RDONLY
   O_WRONLY
   O_CREAT
   
*/
struct file * file_open(const char * path, int mode);
int file_close(struct file * file_ptr);

ssize_t file_size(struct file * file_ptr);


ssize_t file_read(struct file * file_ptr, 
		  void        * buffer, 
		  size_t        length, 
		  loff_t        offset);

ssize_t file_write(struct file * file_ptr, 
		   void        * buffer, 
		   size_t        length, 
		   loff_t        offset);
