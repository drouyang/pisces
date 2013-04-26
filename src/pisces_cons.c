

static char console_buffer[1024*50];
static u64 console_idx = 0;



static ssize_t device_read(struct file *file, char __user *buffer,
			   size_t length, loff_t *offset) {
    int count = 0;
    struct pisces_cons_t * console = &shared_info->console;
    u64 * cons = &console->out_cons;
    u64 * prod = &console->out_prod;

    while (!(*prod == *cons)) {
	//not empty
        console_buffer[console_idx++] = console->out[*cons];
        *cons = (*cons + 1) % PISCES_CONSOLE_SIZE_OUT;
    }

    if (*offset >= console_idx) {
        return 0;
    }

    count = (length < console_idx - *offset) ? length : (console_idx - *offset);
    
    if (copy_to_user(buffer, console_buffer+*offset, count)) {
        return -EFAULT;
    }

    *offset += count;
    return count;
}
