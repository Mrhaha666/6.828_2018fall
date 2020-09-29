	.text

/* Switch from current_thread to next_thread. Make next_thread
 * the current_thread, and set next_thread to 0.
 * Use eax as a temporary register; it is caller saved.
 */
	.globl thread_switch
thread_switch:
	/* YOUR CODE HERE */
	pushal
	movl current_thread, %eax //获取当前线程的 sp 域的地址
	movl %esp, (%eax)   //把当前线程的esp保存到 sp域，以便恢复
		
	movl next_thread, %eax	//获取待调度的线程的 sp 域的地址
	movl %eax, current_thread  // 将待调度线程的地址，赋给current_thread
	movl (%eax), %esp    //恢复待调度线程的保存的sp指针
	popal                 //恢复待调度的线程的通用寄存器
	

	movl $0x0, next_thread

	ret				/* pop return address from stack */
