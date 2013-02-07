
gemini.ko:     file format elf64-x86-64


Disassembly of section .text:

0000000000000000 <device_write>:
   0:	e8 00 00 00 00       	callq  5 <device_write+0x5>
   5:	55                   	push   %rbp
   6:	48 c7 c7 00 00 00 00 	mov    $0x0,%rdi
   d:	31 c0                	xor    %eax,%eax
   f:	48 89 e5             	mov    %rsp,%rbp
  12:	53                   	push   %rbx
  13:	48 89 d3             	mov    %rdx,%rbx
  16:	48 83 ec 08          	sub    $0x8,%rsp
  1a:	e8 00 00 00 00       	callq  1f <device_write+0x1f>
  1f:	48 83 c4 08          	add    $0x8,%rsp
  23:	48 89 d8             	mov    %rbx,%rax
  26:	5b                   	pop    %rbx
  27:	5d                   	pop    %rbp
  28:	c3                   	retq   
  29:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)

0000000000000030 <device_read>:
  30:	e8 00 00 00 00       	callq  35 <device_read+0x5>
  35:	55                   	push   %rbp
  36:	48 c7 c7 00 00 00 00 	mov    $0x0,%rdi
  3d:	31 c0                	xor    %eax,%eax
  3f:	48 89 e5             	mov    %rsp,%rbp
  42:	e8 00 00 00 00       	callq  47 <device_read+0x17>
  47:	31 c0                	xor    %eax,%eax
  49:	5d                   	pop    %rbp
  4a:	c3                   	retq   
  4b:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)

0000000000000050 <device_release>:
  50:	e8 00 00 00 00       	callq  55 <device_release+0x5>
  55:	55                   	push   %rbp
  56:	48 c7 c7 00 00 00 00 	mov    $0x0,%rdi
  5d:	48 89 e5             	mov    %rsp,%rbp
  60:	e8 00 00 00 00       	callq  65 <device_release+0x15>
  65:	31 c0                	xor    %eax,%eax
  67:	5d                   	pop    %rbp
  68:	c3                   	retq   
  69:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)

0000000000000070 <device_open>:
  70:	e8 00 00 00 00       	callq  75 <device_open+0x5>
  75:	55                   	push   %rbp
  76:	48 c7 c7 00 00 00 00 	mov    $0x0,%rdi
  7d:	48 89 e5             	mov    %rsp,%rbp
  80:	e8 00 00 00 00       	callq  85 <device_open+0x15>
  85:	31 c0                	xor    %eax,%eax
  87:	5d                   	pop    %rbp
  88:	c3                   	retq   
  89:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)

0000000000000090 <kick_offline_cpu>:
  90:	e8 00 00 00 00       	callq  95 <kick_offline_cpu+0x5>
  95:	55                   	push   %rbp
  96:	48 89 e5             	mov    %rsp,%rbp
  99:	48 83 ec 10          	sub    $0x10,%rsp
  9d:	48 8b 05 00 00 00 00 	mov    0x0(%rip),%rax        # a4 <kick_offline_cpu+0x14>
  a4:	48 89 5d f0          	mov    %rbx,-0x10(%rbp)
  a8:	4c 89 65 f8          	mov    %r12,-0x8(%rbp)
  ac:	8b 3d 00 00 00 00    	mov    0x0(%rip),%edi        # b2 <kick_offline_cpu+0x22>
  b2:	ff 50 78             	callq  *0x78(%rax)
  b5:	89 c3                	mov    %eax,%ebx
  b7:	48 8b 05 00 00 00 00 	mov    0x0(%rip),%rax        # be <kick_offline_cpu+0x2e>
  be:	48 8b 15 00 00 00 00 	mov    0x0(%rip),%rdx        # c5 <kick_offline_cpu+0x35>
  c5:	89 d9                	mov    %ebx,%ecx
  c7:	48 c7 c7 00 00 00 00 	mov    $0x0,%rdi
  ce:	48 c7 05 00 00 00 00 	movq   $0x0,0x0(%rip)        # d9 <kick_offline_cpu+0x49>
  d5:	00 00 00 00 
  d9:	65 8b 34 25 00 00 00 	mov    %gs:0x0,%esi
  e0:	00 
  e1:	44 8b 60 08          	mov    0x8(%rax),%r12d
  e5:	31 c0                	xor    %eax,%eax
  e7:	e8 00 00 00 00       	callq  ec <kick_offline_cpu+0x5c>
  ec:	89 df                	mov    %ebx,%edi
  ee:	4c 89 e6             	mov    %r12,%rsi
  f1:	e8 00 00 00 00       	callq  f6 <kick_offline_cpu+0x66>
  f6:	48 8b 5d f0          	mov    -0x10(%rbp),%rbx
  fa:	4c 8b 65 f8          	mov    -0x8(%rbp),%r12
  fe:	c9                   	leaveq 
  ff:	c3                   	retq   

0000000000000100 <device_ioctl>:
 100:	e8 00 00 00 00       	callq  105 <device_ioctl+0x5>
 105:	81 ee e8 03 00 00    	sub    $0x3e8,%esi
 10b:	83 fe 04             	cmp    $0x4,%esi
 10e:	76 08                	jbe    118 <device_ioctl+0x18>
 110:	31 c0                	xor    %eax,%eax
 112:	c3                   	retq   
 113:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
 118:	55                   	push   %rbp
 119:	48 89 e5             	mov    %rsp,%rbp
 11c:	41 54                	push   %r12
 11e:	53                   	push   %rbx
 11f:	ff 24 f5 00 00 00 00 	jmpq   *0x0(,%rsi,8)
 126:	66 2e 0f 1f 84 00 00 	nopw   %cs:0x0(%rax,%rax,1)
 12d:	00 00 00 
 130:	48 8b 35 00 00 00 00 	mov    0x0(%rip),%rsi        # 137 <device_ioctl+0x37>
 137:	48 c7 c7 00 00 00 00 	mov    $0x0,%rdi
 13e:	31 c0                	xor    %eax,%eax
 140:	48 89 f2             	mov    %rsi,%rdx
 143:	48 03 15 00 00 00 00 	add    0x0(%rip),%rdx        # 14a <device_ioctl+0x4a>
 14a:	e8 00 00 00 00       	callq  14f <device_ioctl+0x4f>
 14f:	48 8b 35 00 00 00 00 	mov    0x0(%rip),%rsi        # 156 <device_ioctl+0x56>
 156:	48 8b 3d 00 00 00 00 	mov    0x0(%rip),%rdi        # 15d <device_ioctl+0x5d>
 15d:	e8 00 00 00 00       	callq  162 <device_ioctl+0x62>
 162:	48 8b 35 00 00 00 00 	mov    0x0(%rip),%rsi        # 169 <device_ioctl+0x69>
 169:	48 8b 3d 00 00 00 00 	mov    0x0(%rip),%rdi        # 170 <device_ioctl+0x70>
 170:	e8 00 00 00 00       	callq  175 <device_ioctl+0x75>
 175:	48 8d 90 ff 03 00 00 	lea    0x3ff(%rax),%rdx
 17c:	48 85 c0             	test   %rax,%rax
 17f:	4c 8b 05 00 00 00 00 	mov    0x0(%rip),%r8        # 186 <device_ioctl+0x86>
 186:	48 8b 0d 00 00 00 00 	mov    0x0(%rip),%rcx        # 18d <device_ioctl+0x8d>
 18d:	48 89 c6             	mov    %rax,%rsi
 190:	48 c7 c7 00 00 00 00 	mov    $0x0,%rdi
 197:	48 0f 49 d0          	cmovns %rax,%rdx
 19b:	31 c0                	xor    %eax,%eax
 19d:	48 c1 fa 0a          	sar    $0xa,%rdx
 1a1:	e8 00 00 00 00       	callq  1a6 <device_ioctl+0xa6>
 1a6:	5b                   	pop    %rbx
 1a7:	41 5c                	pop    %r12
 1a9:	31 c0                	xor    %eax,%eax
 1ab:	5d                   	pop    %rbp
 1ac:	c3                   	retq   
 1ad:	0f 1f 00             	nopl   (%rax)
 1b0:	48 8b 35 00 00 00 00 	mov    0x0(%rip),%rsi        # 1b7 <device_ioctl+0xb7>
 1b7:	48 c7 c7 00 00 00 00 	mov    $0x0,%rdi
 1be:	31 c0                	xor    %eax,%eax
 1c0:	e8 00 00 00 00       	callq  1c5 <device_ioctl+0xc5>
 1c5:	e8 c6 fe ff ff       	callq  90 <kick_offline_cpu>
 1ca:	5b                   	pop    %rbx
 1cb:	41 5c                	pop    %r12
 1cd:	31 c0                	xor    %eax,%eax
 1cf:	5d                   	pop    %rbp
 1d0:	c3                   	retq   
 1d1:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
 1d8:	4c 8b 25 00 00 00 00 	mov    0x0(%rip),%r12        # 1df <device_ioctl+0xdf>
 1df:	48 c7 c7 00 00 00 00 	mov    $0x0,%rdi
 1e6:	31 c0                	xor    %eax,%eax
 1e8:	48 bb 00 00 00 00 00 	movabs $0xffff880000000000,%rbx
 1ef:	88 ff ff 
 1f2:	4c 89 e6             	mov    %r12,%rsi
 1f5:	4c 01 e3             	add    %r12,%rbx
 1f8:	e8 00 00 00 00       	callq  1fd <device_ioctl+0xfd>
 1fd:	48 b8 50 00 00 00 00 	movabs $0xffff880000000050,%rax
 204:	88 ff ff 
 207:	49 01 c4             	add    %rax,%r12
 20a:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
 210:	48 8b 33             	mov    (%rbx),%rsi
 213:	31 c0                	xor    %eax,%eax
 215:	48 c7 c7 00 00 00 00 	mov    $0x0,%rdi
 21c:	48 83 c3 08          	add    $0x8,%rbx
 220:	e8 00 00 00 00       	callq  225 <device_ioctl+0x125>
 225:	4c 39 e3             	cmp    %r12,%rbx
 228:	75 e6                	jne    210 <device_ioctl+0x110>
 22a:	5b                   	pop    %rbx
 22b:	41 5c                	pop    %r12
 22d:	31 c0                	xor    %eax,%eax
 22f:	5d                   	pop    %rbp
 230:	c3                   	retq   
 231:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
 238:	4c 8b 05 00 00 00 00 	mov    0x0(%rip),%r8        # 23f <device_ioctl+0x13f>
 23f:	48 8b 0d 00 00 00 00 	mov    0x0(%rip),%rcx        # 246 <device_ioctl+0x146>
 246:	48 c7 c7 00 00 00 00 	mov    $0x0,%rdi
 24d:	48 8b 15 00 00 00 00 	mov    0x0(%rip),%rdx        # 254 <device_ioctl+0x154>
 254:	48 8b 35 00 00 00 00 	mov    0x0(%rip),%rsi        # 25b <device_ioctl+0x15b>
 25b:	31 c0                	xor    %eax,%eax
 25d:	e8 00 00 00 00       	callq  262 <device_ioctl+0x162>
 262:	5b                   	pop    %rbx
 263:	41 5c                	pop    %r12
 265:	31 c0                	xor    %eax,%eax
 267:	5d                   	pop    %rbp
 268:	c3                   	retq   
 269:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)

0000000000000270 <hooker>:
 270:	e8 00 00 00 00       	callq  275 <hooker+0x5>
 275:	55                   	push   %rbp
 276:	48 89 e5             	mov    %rsp,%rbp
 279:	53                   	push   %rbx
 27a:	48 83 ec 08          	sub    $0x8,%rsp
 27e:	48 8b 3d 00 00 00 00 	mov    0x0(%rip),%rdi        # 285 <hooker+0x15>
 285:	e8 00 00 00 00       	callq  28a <hooker+0x1a>
 28a:	48 c7 c7 00 00 00 00 	mov    $0x0,%rdi
 291:	48 89 c3             	mov    %rax,%rbx
 294:	31 c0                	xor    %eax,%eax
 296:	e8 00 00 00 00       	callq  29b <hooker+0x2b>
 29b:	48 8b 15 00 00 00 00 	mov    0x0(%rip),%rdx        # 2a2 <hooker+0x32>
 2a2:	48 89 d8             	mov    %rbx,%rax
 2a5:	f4                   	hlt    
 2a6:	0f 22 d8             	mov    %rax,%cr3
 2a9:	48 c7 c0 00 00 00 00 	mov    $0x0,%rax
 2b0:	ff e0                	jmpq   *%rax
 2b2:	48 89 d0             	mov    %rdx,%rax
 2b5:	ff e0                	jmpq   *%rax
 2b7:	48 83 c4 08          	add    $0x8,%rsp
 2bb:	5b                   	pop    %rbx
 2bc:	5d                   	pop    %rbp
 2bd:	c3                   	retq   
 2be:	66 90                	xchg   %ax,%ax

00000000000002c0 <init_module>:
 2c0:	e8 00 00 00 00       	callq  2c5 <init_module+0x5>
 2c5:	55                   	push   %rbp
 2c6:	48 c7 c7 00 00 00 00 	mov    $0x0,%rdi
 2cd:	31 c0                	xor    %eax,%eax
 2cf:	48 89 e5             	mov    %rsp,%rbp
 2d2:	e8 00 00 00 00       	callq  2d7 <init_module+0x17>
 2d7:	31 f6                	xor    %esi,%esi
 2d9:	48 c7 c1 00 00 00 00 	mov    $0x0,%rcx
 2e0:	ba 01 00 00 00       	mov    $0x1,%edx
 2e5:	48 c7 c7 00 00 00 00 	mov    $0x0,%rdi
 2ec:	e8 00 00 00 00       	callq  2f1 <init_module+0x31>
 2f1:	85 c0                	test   %eax,%eax
 2f3:	0f 88 ac 00 00 00    	js     3a5 <init_module+0xe5>
 2f9:	48 c7 c2 00 00 00 00 	mov    $0x0,%rdx
 300:	48 c7 c6 00 00 00 00 	mov    $0x0,%rsi
 307:	48 c7 c7 00 00 00 00 	mov    $0x0,%rdi
 30e:	e8 00 00 00 00       	callq  313 <init_module+0x53>
 313:	48 85 c0             	test   %rax,%rax
 316:	48 89 05 00 00 00 00 	mov    %rax,0x0(%rip)        # 31d <init_module+0x5d>
 31d:	74 76                	je     395 <init_module+0xd5>
 31f:	8b 15 00 00 00 00    	mov    0x0(%rip),%edx        # 325 <init_module+0x65>
 325:	31 c9                	xor    %ecx,%ecx
 327:	31 f6                	xor    %esi,%esi
 329:	48 89 c7             	mov    %rax,%rdi
 32c:	49 c7 c0 00 00 00 00 	mov    $0x0,%r8
 333:	31 c0                	xor    %eax,%eax
 335:	e8 00 00 00 00       	callq  33a <init_module+0x7a>
 33a:	48 85 c0             	test   %rax,%rax
 33d:	74 4a                	je     389 <init_module+0xc9>
 33f:	48 c7 c6 00 00 00 00 	mov    $0x0,%rsi
 346:	48 c7 c7 00 00 00 00 	mov    $0x0,%rdi
 34d:	e8 00 00 00 00       	callq  352 <init_module+0x92>
 352:	8b 35 00 00 00 00    	mov    0x0(%rip),%esi        # 358 <init_module+0x98>
 358:	ba 01 00 00 00       	mov    $0x1,%edx
 35d:	48 c7 c7 00 00 00 00 	mov    $0x0,%rdi
 364:	e8 00 00 00 00       	callq  369 <init_module+0xa9>
 369:	83 f8 ff             	cmp    $0xffffffff,%eax
 36c:	74 09                	je     377 <init_module+0xb7>
 36e:	e8 1d fd ff ff       	callq  90 <kick_offline_cpu>
 373:	31 c0                	xor    %eax,%eax
 375:	5d                   	pop    %rbp
 376:	c3                   	retq   
 377:	8b 35 00 00 00 00    	mov    0x0(%rip),%esi        # 37d <init_module+0xbd>
 37d:	48 8b 3d 00 00 00 00 	mov    0x0(%rip),%rdi        # 384 <init_module+0xc4>
 384:	e8 00 00 00 00       	callq  389 <init_module+0xc9>
 389:	48 8b 3d 00 00 00 00 	mov    0x0(%rip),%rdi        # 390 <init_module+0xd0>
 390:	e8 00 00 00 00       	callq  395 <init_module+0xd5>
 395:	8b 3d 00 00 00 00    	mov    0x0(%rip),%edi        # 39b <init_module+0xdb>
 39b:	be 01 00 00 00       	mov    $0x1,%esi
 3a0:	e8 00 00 00 00       	callq  3a5 <init_module+0xe5>
 3a5:	48 c7 c7 00 00 00 00 	mov    $0x0,%rdi
 3ac:	31 c0                	xor    %eax,%eax
 3ae:	e8 00 00 00 00       	callq  3b3 <init_module+0xf3>
 3b3:	b8 ff ff ff ff       	mov    $0xffffffff,%eax
 3b8:	5d                   	pop    %rbp
 3b9:	c3                   	retq   
 3ba:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)

00000000000003c0 <cleanup_module>:
 3c0:	e8 00 00 00 00       	callq  3c5 <cleanup_module+0x5>
 3c5:	55                   	push   %rbp
 3c6:	48 c7 c7 00 00 00 00 	mov    $0x0,%rdi
 3cd:	48 89 e5             	mov    %rsp,%rbp
 3d0:	e8 00 00 00 00       	callq  3d5 <cleanup_module+0x15>
 3d5:	8b 35 00 00 00 00    	mov    0x0(%rip),%esi        # 3db <cleanup_module+0x1b>
 3db:	48 8b 3d 00 00 00 00 	mov    0x0(%rip),%rdi        # 3e2 <cleanup_module+0x22>
 3e2:	e8 00 00 00 00       	callq  3e7 <cleanup_module+0x27>
 3e7:	48 8b 3d 00 00 00 00 	mov    0x0(%rip),%rdi        # 3ee <cleanup_module+0x2e>
 3ee:	e8 00 00 00 00       	callq  3f3 <cleanup_module+0x33>
 3f3:	8b 3d 00 00 00 00    	mov    0x0(%rip),%edi        # 3f9 <cleanup_module+0x39>
 3f9:	be 01 00 00 00       	mov    $0x1,%esi
 3fe:	e8 00 00 00 00       	callq  403 <cleanup_module+0x43>
 403:	48 c7 c7 00 00 00 00 	mov    $0x0,%rdi
 40a:	31 c0                	xor    %eax,%eax
 40c:	e8 00 00 00 00       	callq  411 <cleanup_module+0x51>
 411:	5d                   	pop    %rbp
 412:	c3                   	retq   
 413:	66 2e 0f 1f 84 00 00 	nopw   %cs:0x0(%rax,%rax,1)
 41a:	00 00 00 
 41d:	0f 1f 00             	nopl   (%rax)

0000000000000420 <lapic_get_maxlvt>:
 420:	e8 00 00 00 00       	callq  425 <lapic_get_maxlvt+0x5>
 425:	48 8b 05 00 00 00 00 	mov    0x0(%rip),%rax        # 42c <lapic_get_maxlvt+0xc>
 42c:	55                   	push   %rbp
 42d:	bf 30 00 00 00       	mov    $0x30,%edi
 432:	48 89 e5             	mov    %rsp,%rbp
 435:	ff 90 20 01 00 00    	callq  *0x120(%rax)
 43b:	c1 e8 10             	shr    $0x10,%eax
 43e:	25 ff 00 00 00       	and    $0xff,%eax
 443:	5d                   	pop    %rbp
 444:	c3                   	retq   
 445:	66 66 2e 0f 1f 84 00 	data32 nopw %cs:0x0(%rax,%rax,1)
 44c:	00 00 00 00 

0000000000000450 <wakeup_secondary_cpu_via_init>:
 450:	e8 00 00 00 00       	callq  455 <wakeup_secondary_cpu_via_init+0x5>
 455:	55                   	push   %rbp
 456:	48 89 e5             	mov    %rsp,%rbp
 459:	41 57                	push   %r15
 45b:	49 89 f7             	mov    %rsi,%r15
 45e:	41 56                	push   %r14
 460:	41 89 fe             	mov    %edi,%r14d
 463:	bf 30 00 00 00       	mov    $0x30,%edi
 468:	41 55                	push   %r13
 46a:	41 54                	push   %r12
 46c:	53                   	push   %rbx
 46d:	48 83 ec 18          	sub    $0x18,%rsp
 471:	48 8b 05 00 00 00 00 	mov    0x0(%rip),%rax        # 478 <wakeup_secondary_cpu_via_init+0x28>
 478:	ff 90 20 01 00 00    	callq  *0x120(%rax)
 47e:	c1 e8 10             	shr    $0x10,%eax
 481:	44 0f b6 e8          	movzbl %al,%r13d
 485:	41 83 fd 03          	cmp    $0x3,%r13d
 489:	0f 8f f1 01 00 00    	jg     680 <wakeup_secondary_cpu_via_init+0x230>
 48f:	48 8b 05 00 00 00 00 	mov    0x0(%rip),%rax        # 496 <wakeup_secondary_cpu_via_init+0x46>
 496:	bf 80 02 00 00       	mov    $0x280,%edi
 49b:	ff 90 20 01 00 00    	callq  *0x120(%rax)
 4a1:	f6 05 00 00 00 00 04 	testb  $0x4,0x0(%rip)        # 4a8 <wakeup_secondary_cpu_via_init+0x58>
 4a8:	0f 85 fe 02 00 00    	jne    7ac <wakeup_secondary_cpu_via_init+0x35c>
 4ae:	48 8b 05 00 00 00 00 	mov    0x0(%rip),%rax        # 4b5 <wakeup_secondary_cpu_via_init+0x65>
 4b5:	44 89 f6             	mov    %r14d,%esi
 4b8:	bf 00 c5 00 00       	mov    $0xc500,%edi
 4bd:	ff 90 40 01 00 00    	callq  *0x140(%rax)
 4c3:	f6 05 00 00 00 00 04 	testb  $0x4,0x0(%rip)        # 4ca <wakeup_secondary_cpu_via_init+0x7a>
 4ca:	0f 85 88 02 00 00    	jne    758 <wakeup_secondary_cpu_via_init+0x308>
 4d0:	48 8b 05 00 00 00 00 	mov    0x0(%rip),%rax        # 4d7 <wakeup_secondary_cpu_via_init+0x87>
 4d7:	bb 0a 00 00 00       	mov    $0xa,%ebx
 4dc:	ff 90 50 01 00 00    	callq  *0x150(%rax)
 4e2:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
 4e8:	bf 58 89 41 00       	mov    $0x418958,%edi
 4ed:	e8 00 00 00 00       	callq  4f2 <wakeup_secondary_cpu_via_init+0xa2>
 4f2:	48 83 eb 01          	sub    $0x1,%rbx
 4f6:	75 f0                	jne    4e8 <wakeup_secondary_cpu_via_init+0x98>
 4f8:	f6 05 00 00 00 00 04 	testb  $0x4,0x0(%rip)        # 4ff <wakeup_secondary_cpu_via_init+0xaf>
 4ff:	0f 85 c1 02 00 00    	jne    7c6 <wakeup_secondary_cpu_via_init+0x376>
 505:	48 8b 05 00 00 00 00 	mov    0x0(%rip),%rax        # 50c <wakeup_secondary_cpu_via_init+0xbc>
 50c:	44 89 f6             	mov    %r14d,%esi
 50f:	bf 00 85 00 00       	mov    $0x8500,%edi
 514:	ff 90 40 01 00 00    	callq  *0x140(%rax)
 51a:	f6 05 00 00 00 00 04 	testb  $0x4,0x0(%rip)        # 521 <wakeup_secondary_cpu_via_init+0xd1>
 521:	0f 85 4b 02 00 00    	jne    772 <wakeup_secondary_cpu_via_init+0x322>
 527:	48 8b 05 00 00 00 00 	mov    0x0(%rip),%rax        # 52e <wakeup_secondary_cpu_via_init+0xde>
 52e:	ff 90 50 01 00 00    	callq  *0x150(%rax)
 534:	0f ae f0             	mfence 
 537:	f6 05 00 00 00 00 04 	testb  $0x4,0x0(%rip)        # 53e <wakeup_secondary_cpu_via_init+0xee>
 53e:	0f 85 f5 01 00 00    	jne    739 <wakeup_secondary_cpu_via_init+0x2e9>
 544:	49 c1 ef 0c          	shr    $0xc,%r15
 548:	41 bc 01 00 00 00    	mov    $0x1,%r12d
 54e:	41 81 cf 00 06 00 00 	or     $0x600,%r15d
 555:	f6 05 00 00 00 00 04 	testb  $0x4,0x0(%rip)        # 55c <wakeup_secondary_cpu_via_init+0x10c>
 55c:	0f 85 85 01 00 00    	jne    6e7 <wakeup_secondary_cpu_via_init+0x297>
 562:	41 83 fd 03          	cmp    $0x3,%r13d
 566:	0f 8f f4 00 00 00    	jg     660 <wakeup_secondary_cpu_via_init+0x210>
 56c:	48 8b 05 00 00 00 00 	mov    0x0(%rip),%rax        # 573 <wakeup_secondary_cpu_via_init+0x123>
 573:	bf 80 02 00 00       	mov    $0x280,%edi
 578:	ff 90 20 01 00 00    	callq  *0x120(%rax)
 57e:	f6 05 00 00 00 00 04 	testb  $0x4,0x0(%rip)        # 585 <wakeup_secondary_cpu_via_init+0x135>
 585:	0f 85 42 01 00 00    	jne    6cd <wakeup_secondary_cpu_via_init+0x27d>
 58b:	48 8b 05 00 00 00 00 	mov    0x0(%rip),%rax        # 592 <wakeup_secondary_cpu_via_init+0x142>
 592:	44 89 f6             	mov    %r14d,%esi
 595:	44 89 ff             	mov    %r15d,%edi
 598:	ff 90 40 01 00 00    	callq  *0x140(%rax)
 59e:	bf 34 a9 13 00       	mov    $0x13a934,%edi
 5a3:	e8 00 00 00 00       	callq  5a8 <wakeup_secondary_cpu_via_init+0x158>
 5a8:	f6 05 00 00 00 00 04 	testb  $0x4,0x0(%rip)        # 5af <wakeup_secondary_cpu_via_init+0x15f>
 5af:	0f 85 fe 00 00 00    	jne    6b3 <wakeup_secondary_cpu_via_init+0x263>
 5b5:	f6 05 00 00 00 00 04 	testb  $0x4,0x0(%rip)        # 5bc <wakeup_secondary_cpu_via_init+0x16c>
 5bc:	0f 85 d7 00 00 00    	jne    699 <wakeup_secondary_cpu_via_init+0x249>
 5c2:	48 8b 05 00 00 00 00 	mov    0x0(%rip),%rax        # 5c9 <wakeup_secondary_cpu_via_init+0x179>
 5c9:	ff 90 50 01 00 00    	callq  *0x150(%rax)
 5cf:	bf 78 1b 0d 00       	mov    $0xd1b78,%edi
 5d4:	89 c3                	mov    %eax,%ebx
 5d6:	e8 00 00 00 00       	callq  5db <wakeup_secondary_cpu_via_init+0x18b>
 5db:	41 83 fd 03          	cmp    $0x3,%r13d
 5df:	7f 67                	jg     648 <wakeup_secondary_cpu_via_init+0x1f8>
 5e1:	48 8b 05 00 00 00 00 	mov    0x0(%rip),%rax        # 5e8 <wakeup_secondary_cpu_via_init+0x198>
 5e8:	bf 80 02 00 00       	mov    $0x280,%edi
 5ed:	ff 90 20 01 00 00    	callq  *0x120(%rax)
 5f3:	25 ef 00 00 00       	and    $0xef,%eax
 5f8:	89 c2                	mov    %eax,%edx
 5fa:	09 da                	or     %ebx,%edx
 5fc:	75 12                	jne    610 <wakeup_secondary_cpu_via_init+0x1c0>
 5fe:	41 83 c4 01          	add    $0x1,%r12d
 602:	41 83 fc 03          	cmp    $0x3,%r12d
 606:	0f 85 49 ff ff ff    	jne    555 <wakeup_secondary_cpu_via_init+0x105>
 60c:	31 db                	xor    %ebx,%ebx
 60e:	31 c0                	xor    %eax,%eax
 610:	f6 05 00 00 00 00 04 	testb  $0x4,0x0(%rip)        # 617 <wakeup_secondary_cpu_via_init+0x1c7>
 617:	41 89 c4             	mov    %eax,%r12d
 61a:	0f 85 6c 01 00 00    	jne    78c <wakeup_secondary_cpu_via_init+0x33c>
 620:	85 db                	test   %ebx,%ebx
 622:	0f 85 f8 00 00 00    	jne    720 <wakeup_secondary_cpu_via_init+0x2d0>
 628:	4d 85 e4             	test   %r12,%r12
 62b:	0f 85 d3 00 00 00    	jne    704 <wakeup_secondary_cpu_via_init+0x2b4>
 631:	48 83 c4 18          	add    $0x18,%rsp
 635:	89 d0                	mov    %edx,%eax
 637:	5b                   	pop    %rbx
 638:	41 5c                	pop    %r12
 63a:	41 5d                	pop    %r13
 63c:	41 5e                	pop    %r14
 63e:	41 5f                	pop    %r15
 640:	5d                   	pop    %rbp
 641:	c3                   	retq   
 642:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
 648:	48 8b 05 00 00 00 00 	mov    0x0(%rip),%rax        # 64f <wakeup_secondary_cpu_via_init+0x1ff>
 64f:	31 f6                	xor    %esi,%esi
 651:	bf 80 02 00 00       	mov    $0x280,%edi
 656:	ff 90 28 01 00 00    	callq  *0x128(%rax)
 65c:	eb 83                	jmp    5e1 <wakeup_secondary_cpu_via_init+0x191>
 65e:	66 90                	xchg   %ax,%ax
 660:	48 8b 05 00 00 00 00 	mov    0x0(%rip),%rax        # 667 <wakeup_secondary_cpu_via_init+0x217>
 667:	31 f6                	xor    %esi,%esi
 669:	bf 80 02 00 00       	mov    $0x280,%edi
 66e:	ff 90 28 01 00 00    	callq  *0x128(%rax)
 674:	e9 f3 fe ff ff       	jmpq   56c <wakeup_secondary_cpu_via_init+0x11c>
 679:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
 680:	48 8b 05 00 00 00 00 	mov    0x0(%rip),%rax        # 687 <wakeup_secondary_cpu_via_init+0x237>
 687:	31 f6                	xor    %esi,%esi
 689:	bf 80 02 00 00       	mov    $0x280,%edi
 68e:	ff 90 28 01 00 00    	callq  *0x128(%rax)
 694:	e9 f6 fd ff ff       	jmpq   48f <wakeup_secondary_cpu_via_init+0x3f>
 699:	48 c7 c6 00 00 00 00 	mov    $0x0,%rsi
 6a0:	48 c7 c7 00 00 00 00 	mov    $0x0,%rdi
 6a7:	31 c0                	xor    %eax,%eax
 6a9:	e8 00 00 00 00       	callq  6ae <wakeup_secondary_cpu_via_init+0x25e>
 6ae:	e9 0f ff ff ff       	jmpq   5c2 <wakeup_secondary_cpu_via_init+0x172>
 6b3:	48 c7 c6 00 00 00 00 	mov    $0x0,%rsi
 6ba:	48 c7 c7 00 00 00 00 	mov    $0x0,%rdi
 6c1:	31 c0                	xor    %eax,%eax
 6c3:	e8 00 00 00 00       	callq  6c8 <wakeup_secondary_cpu_via_init+0x278>
 6c8:	e9 e8 fe ff ff       	jmpq   5b5 <wakeup_secondary_cpu_via_init+0x165>
 6cd:	48 c7 c6 00 00 00 00 	mov    $0x0,%rsi
 6d4:	48 c7 c7 00 00 00 00 	mov    $0x0,%rdi
 6db:	31 c0                	xor    %eax,%eax
 6dd:	e8 00 00 00 00       	callq  6e2 <wakeup_secondary_cpu_via_init+0x292>
 6e2:	e9 a4 fe ff ff       	jmpq   58b <wakeup_secondary_cpu_via_init+0x13b>
 6e7:	44 89 e2             	mov    %r12d,%edx
 6ea:	48 c7 c6 00 00 00 00 	mov    $0x0,%rsi
 6f1:	48 c7 c7 00 00 00 00 	mov    $0x0,%rdi
 6f8:	31 c0                	xor    %eax,%eax
 6fa:	e8 00 00 00 00       	callq  6ff <wakeup_secondary_cpu_via_init+0x2af>
 6ff:	e9 5e fe ff ff       	jmpq   562 <wakeup_secondary_cpu_via_init+0x112>
 704:	4c 89 e6             	mov    %r12,%rsi
 707:	48 c7 c7 00 00 00 00 	mov    $0x0,%rdi
 70e:	31 c0                	xor    %eax,%eax
 710:	89 55 c8             	mov    %edx,-0x38(%rbp)
 713:	e8 00 00 00 00       	callq  718 <wakeup_secondary_cpu_via_init+0x2c8>
 718:	8b 55 c8             	mov    -0x38(%rbp),%edx
 71b:	e9 11 ff ff ff       	jmpq   631 <wakeup_secondary_cpu_via_init+0x1e1>
 720:	48 c7 c7 00 00 00 00 	mov    $0x0,%rdi
 727:	31 c0                	xor    %eax,%eax
 729:	89 55 c8             	mov    %edx,-0x38(%rbp)
 72c:	e8 00 00 00 00       	callq  731 <wakeup_secondary_cpu_via_init+0x2e1>
 731:	8b 55 c8             	mov    -0x38(%rbp),%edx
 734:	e9 ef fe ff ff       	jmpq   628 <wakeup_secondary_cpu_via_init+0x1d8>
 739:	ba 02 00 00 00       	mov    $0x2,%edx
 73e:	48 c7 c6 00 00 00 00 	mov    $0x0,%rsi
 745:	48 c7 c7 00 00 00 00 	mov    $0x0,%rdi
 74c:	31 c0                	xor    %eax,%eax
 74e:	e8 00 00 00 00       	callq  753 <wakeup_secondary_cpu_via_init+0x303>
 753:	e9 ec fd ff ff       	jmpq   544 <wakeup_secondary_cpu_via_init+0xf4>
 758:	48 c7 c6 00 00 00 00 	mov    $0x0,%rsi
 75f:	48 c7 c7 00 00 00 00 	mov    $0x0,%rdi
 766:	31 c0                	xor    %eax,%eax
 768:	e8 00 00 00 00       	callq  76d <wakeup_secondary_cpu_via_init+0x31d>
 76d:	e9 5e fd ff ff       	jmpq   4d0 <wakeup_secondary_cpu_via_init+0x80>
 772:	48 c7 c6 00 00 00 00 	mov    $0x0,%rsi
 779:	48 c7 c7 00 00 00 00 	mov    $0x0,%rdi
 780:	31 c0                	xor    %eax,%eax
 782:	e8 00 00 00 00       	callq  787 <wakeup_secondary_cpu_via_init+0x337>
 787:	e9 9b fd ff ff       	jmpq   527 <wakeup_secondary_cpu_via_init+0xd7>
 78c:	48 c7 c6 00 00 00 00 	mov    $0x0,%rsi
 793:	48 c7 c7 00 00 00 00 	mov    $0x0,%rdi
 79a:	31 c0                	xor    %eax,%eax
 79c:	89 55 c8             	mov    %edx,-0x38(%rbp)
 79f:	e8 00 00 00 00       	callq  7a4 <wakeup_secondary_cpu_via_init+0x354>
 7a4:	8b 55 c8             	mov    -0x38(%rbp),%edx
 7a7:	e9 74 fe ff ff       	jmpq   620 <wakeup_secondary_cpu_via_init+0x1d0>
 7ac:	48 c7 c6 00 00 00 00 	mov    $0x0,%rsi
 7b3:	48 c7 c7 00 00 00 00 	mov    $0x0,%rdi
 7ba:	31 c0                	xor    %eax,%eax
 7bc:	e8 00 00 00 00       	callq  7c1 <wakeup_secondary_cpu_via_init+0x371>
 7c1:	e9 e8 fc ff ff       	jmpq   4ae <wakeup_secondary_cpu_via_init+0x5e>
 7c6:	48 c7 c6 00 00 00 00 	mov    $0x0,%rsi
 7cd:	48 c7 c7 00 00 00 00 	mov    $0x0,%rdi
 7d4:	31 c0                	xor    %eax,%eax
 7d6:	e8 00 00 00 00       	callq  7db <wakeup_secondary_cpu_via_init+0x38b>
 7db:	e9 25 fd ff ff       	jmpq   505 <wakeup_secondary_cpu_via_init+0xb5>

00000000000007e0 <map_offline_memory>:
 7e0:	e8 00 00 00 00       	callq  7e5 <map_offline_memory+0x5>
 7e5:	55                   	push   %rbp
 7e6:	48 89 e5             	mov    %rsp,%rbp
 7e9:	5d                   	pop    %rbp
 7ea:	c3                   	retq   
 7eb:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)

00000000000007f0 <bootstrap_pgtable_init>:
 7f0:	e8 00 00 00 00       	callq  7f5 <bootstrap_pgtable_init+0x5>
 7f5:	55                   	push   %rbp
 7f6:	be 02 00 00 00       	mov    $0x2,%esi
 7fb:	48 89 e5             	mov    %rsp,%rbp
 7fe:	41 55                	push   %r13
 800:	41 54                	push   %r12
 802:	49 89 fc             	mov    %rdi,%r12
 805:	bf d0 00 00 00       	mov    $0xd0,%edi
 80a:	53                   	push   %rbx
 80b:	4c 89 e3             	mov    %r12,%rbx
 80e:	48 c1 eb 27          	shr    $0x27,%rbx
 812:	48 83 ec 08          	sub    $0x8,%rsp
 816:	81 e3 ff 01 00 00    	and    $0x1ff,%ebx
 81c:	e8 00 00 00 00       	callq  821 <bootstrap_pgtable_init+0x31>
 821:	31 f6                	xor    %esi,%esi
 823:	ba 00 30 00 00       	mov    $0x3000,%edx
 828:	48 89 c7             	mov    %rax,%rdi
 82b:	48 89 05 00 00 00 00 	mov    %rax,0x0(%rip)        # 832 <bootstrap_pgtable_init+0x42>
 832:	e8 00 00 00 00       	callq  837 <bootstrap_pgtable_init+0x47>
 837:	4c 8b 2d 00 00 00 00 	mov    0x0(%rip),%r13        # 83e <bootstrap_pgtable_init+0x4e>
 83e:	49 8d bd 00 10 00 00 	lea    0x1000(%r13),%rdi
 845:	e8 00 00 00 00       	callq  84a <bootstrap_pgtable_init+0x5a>
 84a:	41 0f b6 54 dd 01    	movzbl 0x1(%r13,%rbx,8),%edx
 850:	48 89 c1             	mov    %rax,%rcx
 853:	48 c1 e9 15          	shr    $0x15,%rcx
 857:	c1 e1 04             	shl    $0x4,%ecx
 85a:	83 e2 0f             	and    $0xf,%edx
 85d:	09 ca                	or     %ecx,%edx
 85f:	41 88 54 dd 01       	mov    %dl,0x1(%r13,%rbx,8)
 864:	48 89 c2             	mov    %rax,%rdx
 867:	48 c1 ea 19          	shr    $0x19,%rdx
 86b:	41 88 54 dd 02       	mov    %dl,0x2(%r13,%rbx,8)
 870:	48 89 c2             	mov    %rax,%rdx
 873:	48 c1 ea 21          	shr    $0x21,%rdx
 877:	41 88 54 dd 03       	mov    %dl,0x3(%r13,%rbx,8)
 87c:	48 89 c2             	mov    %rax,%rdx
 87f:	48 c1 ea 29          	shr    $0x29,%rdx
 883:	41 88 54 dd 04       	mov    %dl,0x4(%r13,%rbx,8)
 888:	48 89 c2             	mov    %rax,%rdx
 88b:	48 c1 e8 39          	shr    $0x39,%rax
 88f:	48 c1 ea 31          	shr    $0x31,%rdx
 893:	83 e0 0f             	and    $0xf,%eax
 896:	41 88 54 dd 05       	mov    %dl,0x5(%r13,%rbx,8)
 89b:	41 0f b6 54 dd 06    	movzbl 0x6(%r13,%rbx,8),%edx
 8a1:	83 e2 f0             	and    $0xfffffff0,%edx
 8a4:	09 c2                	or     %eax,%edx
 8a6:	41 88 54 dd 06       	mov    %dl,0x6(%r13,%rbx,8)
 8ab:	48 8b 05 00 00 00 00 	mov    0x0(%rip),%rax        # 8b2 <bootstrap_pgtable_init+0xc2>
 8b2:	4d 89 e5             	mov    %r12,%r13
 8b5:	49 c1 ed 1e          	shr    $0x1e,%r13
 8b9:	41 81 e5 ff 01 00 00 	and    $0x1ff,%r13d
 8c0:	80 0c d8 01          	orb    $0x1,(%rax,%rbx,8)
 8c4:	48 8b 05 00 00 00 00 	mov    0x0(%rip),%rax        # 8cb <bootstrap_pgtable_init+0xdb>
 8cb:	80 0c d8 02          	orb    $0x2,(%rax,%rbx,8)
 8cf:	48 8b 05 00 00 00 00 	mov    0x0(%rip),%rax        # 8d6 <bootstrap_pgtable_init+0xe6>
 8d6:	80 0c d8 20          	orb    $0x20,(%rax,%rbx,8)
 8da:	48 8b 1d 00 00 00 00 	mov    0x0(%rip),%rbx        # 8e1 <bootstrap_pgtable_init+0xf1>
 8e1:	48 8d bb 00 20 00 00 	lea    0x2000(%rbx),%rdi
 8e8:	e8 00 00 00 00       	callq  8ed <bootstrap_pgtable_init+0xfd>
 8ed:	49 8d 95 00 02 00 00 	lea    0x200(%r13),%rdx
 8f4:	48 89 c6             	mov    %rax,%rsi
 8f7:	48 c1 ee 15          	shr    $0x15,%rsi
 8fb:	0f b6 4c d3 01       	movzbl 0x1(%rbx,%rdx,8),%ecx
 900:	c1 e6 04             	shl    $0x4,%esi
 903:	83 e1 0f             	and    $0xf,%ecx
 906:	09 f1                	or     %esi,%ecx
 908:	88 4c d3 01          	mov    %cl,0x1(%rbx,%rdx,8)
 90c:	48 89 c1             	mov    %rax,%rcx
 90f:	48 c1 e9 19          	shr    $0x19,%rcx
 913:	88 4c d3 02          	mov    %cl,0x2(%rbx,%rdx,8)
 917:	48 89 c1             	mov    %rax,%rcx
 91a:	48 c1 e9 21          	shr    $0x21,%rcx
 91e:	88 4c d3 03          	mov    %cl,0x3(%rbx,%rdx,8)
 922:	48 89 c1             	mov    %rax,%rcx
 925:	48 c1 e9 29          	shr    $0x29,%rcx
 929:	88 4c d3 04          	mov    %cl,0x4(%rbx,%rdx,8)
 92d:	48 89 c1             	mov    %rax,%rcx
 930:	48 c1 e9 31          	shr    $0x31,%rcx
 934:	48 c1 e8 39          	shr    $0x39,%rax
 938:	45 31 ed             	xor    %r13d,%r13d
 93b:	88 4c d3 05          	mov    %cl,0x5(%rbx,%rdx,8)
 93f:	0f b6 4c d3 06       	movzbl 0x6(%rbx,%rdx,8),%ecx
 944:	83 e0 0f             	and    $0xf,%eax
 947:	83 e1 f0             	and    $0xfffffff0,%ecx
 94a:	09 c1                	or     %eax,%ecx
 94c:	88 4c d3 06          	mov    %cl,0x6(%rbx,%rdx,8)
 950:	48 8b 1d 00 00 00 00 	mov    0x0(%rip),%rbx        # 957 <bootstrap_pgtable_init+0x167>
 957:	80 0c d3 23          	orb    $0x23,(%rbx,%rdx,8)
 95b:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
 960:	44 89 ef             	mov    %r13d,%edi
 963:	c1 e7 15             	shl    $0x15,%edi
 966:	48 63 ff             	movslq %edi,%rdi
 969:	4c 01 e7             	add    %r12,%rdi
 96c:	e8 00 00 00 00       	callq  971 <bootstrap_pgtable_init+0x181>
 971:	49 63 d5             	movslq %r13d,%rdx
 974:	48 89 c6             	mov    %rax,%rsi
 977:	41 83 c5 01          	add    $0x1,%r13d
 97b:	48 81 c2 00 04 00 00 	add    $0x400,%rdx
 982:	48 c1 ee 15          	shr    $0x15,%rsi
 986:	0f b6 4c d3 02       	movzbl 0x2(%rbx,%rdx,8),%ecx
 98b:	c1 e6 05             	shl    $0x5,%esi
 98e:	83 e1 1f             	and    $0x1f,%ecx
 991:	09 f1                	or     %esi,%ecx
 993:	88 4c d3 02          	mov    %cl,0x2(%rbx,%rdx,8)
 997:	48 89 c1             	mov    %rax,%rcx
 99a:	48 c1 e9 18          	shr    $0x18,%rcx
 99e:	88 4c d3 03          	mov    %cl,0x3(%rbx,%rdx,8)
 9a2:	48 89 c1             	mov    %rax,%rcx
 9a5:	48 c1 e9 20          	shr    $0x20,%rcx
 9a9:	88 4c d3 04          	mov    %cl,0x4(%rbx,%rdx,8)
 9ad:	48 89 c1             	mov    %rax,%rcx
 9b0:	48 c1 e8 30          	shr    $0x30,%rax
 9b4:	48 c1 e9 28          	shr    $0x28,%rcx
 9b8:	83 e0 0f             	and    $0xf,%eax
 9bb:	88 4c d3 05          	mov    %cl,0x5(%rbx,%rdx,8)
 9bf:	0f b6 4c d3 06       	movzbl 0x6(%rbx,%rdx,8),%ecx
 9c4:	83 e1 f0             	and    $0xfffffff0,%ecx
 9c7:	09 c1                	or     %eax,%ecx
 9c9:	88 4c d3 06          	mov    %cl,0x6(%rbx,%rdx,8)
 9cd:	48 8b 1d 00 00 00 00 	mov    0x0(%rip),%rbx        # 9d4 <bootstrap_pgtable_init+0x1e4>
 9d4:	80 0c d3 a3          	orb    $0xa3,(%rbx,%rdx,8)
 9d8:	41 81 fd 00 02 00 00 	cmp    $0x200,%r13d
 9df:	0f 85 7b ff ff ff    	jne    960 <bootstrap_pgtable_init+0x170>
 9e5:	48 83 c4 08          	add    $0x8,%rsp
 9e9:	5b                   	pop    %rbx
 9ea:	41 5c                	pop    %r12
 9ec:	41 5d                	pop    %r13
 9ee:	5d                   	pop    %rbp
 9ef:	c3                   	retq   

00000000000009f0 <load_image>:
 9f0:	e8 00 00 00 00       	callq  9f5 <load_image+0x5>
 9f5:	55                   	push   %rbp
 9f6:	48 89 e5             	mov    %rsp,%rbp
 9f9:	48 83 ec 30          	sub    $0x30,%rsp
 9fd:	48 85 ff             	test   %rdi,%rdi
 a00:	48 89 5d e0          	mov    %rbx,-0x20(%rbp)
 a04:	4c 89 65 e8          	mov    %r12,-0x18(%rbp)
 a08:	4c 89 6d f0          	mov    %r13,-0x10(%rbp)
 a0c:	4c 89 75 f8          	mov    %r14,-0x8(%rbp)
 a10:	48 c7 45 d8 00 00 00 	movq   $0x0,-0x28(%rbp)
 a17:	00 
 a18:	0f 84 85 00 00 00    	je     aa3 <load_image+0xb3>
 a1e:	65 4c 8b 24 25 00 00 	mov    %gs:0x0,%r12
 a25:	00 00 
 a27:	31 d2                	xor    %edx,%edx
 a29:	4d 8b b4 24 48 e0 ff 	mov    -0x1fb8(%r12),%r14
 a30:	ff 
 a31:	49 89 f5             	mov    %rsi,%r13
 a34:	49 c7 84 24 48 e0 ff 	movq   $0xffffffffffffffff,-0x1fb8(%r12)
 a3b:	ff ff ff ff ff 
 a40:	be 02 00 00 00       	mov    $0x2,%esi
 a45:	e8 00 00 00 00       	callq  a4a <load_image+0x5a>
 a4a:	48 3d 00 f0 ff ff    	cmp    $0xfffffffffffff000,%rax
 a50:	48 89 c3             	mov    %rax,%rbx
 a53:	77 50                	ja     aa5 <load_image+0xb5>
 a55:	48 8d 4d d8          	lea    -0x28(%rbp),%rcx
 a59:	48 be 00 00 00 00 00 	movabs $0xffff880000000000,%rsi
 a60:	88 ff ff 
 a63:	ba 00 00 00 20       	mov    $0x20000000,%edx
 a68:	4c 01 ee             	add    %r13,%rsi
 a6b:	48 89 c7             	mov    %rax,%rdi
 a6e:	48 c7 45 d8 00 00 00 	movq   $0x0,-0x28(%rbp)
 a75:	00 
 a76:	e8 00 00 00 00       	callq  a7b <load_image+0x8b>
 a7b:	4d 89 b4 24 48 e0 ff 	mov    %r14,-0x1fb8(%r12)
 a82:	ff 
 a83:	31 f6                	xor    %esi,%esi
 a85:	48 89 df             	mov    %rbx,%rdi
 a88:	e8 00 00 00 00       	callq  a8d <load_image+0x9d>
 a8d:	48 8b 45 d8          	mov    -0x28(%rbp),%rax
 a91:	48 8b 5d e0          	mov    -0x20(%rbp),%rbx
 a95:	4c 8b 65 e8          	mov    -0x18(%rbp),%r12
 a99:	4c 8b 6d f0          	mov    -0x10(%rbp),%r13
 a9d:	4c 8b 75 f8          	mov    -0x8(%rbp),%r14
 aa1:	c9                   	leaveq 
 aa2:	c3                   	retq   
 aa3:	0f 0b                	ud2    
 aa5:	31 c0                	xor    %eax,%eax
 aa7:	eb e8                	jmp    a91 <load_image+0xa1>
 aa9:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)

0000000000000ab0 <loader_exit>:
 ab0:	e8 00 00 00 00       	callq  ab5 <loader_exit+0x5>
 ab5:	55                   	push   %rbp
 ab6:	48 8b 3d 00 00 00 00 	mov    0x0(%rip),%rdi        # abd <loader_exit+0xd>
 abd:	be 02 00 00 00       	mov    $0x2,%esi
 ac2:	48 89 e5             	mov    %rsp,%rbp
 ac5:	e8 00 00 00 00       	callq  aca <loader_exit+0x1a>
 aca:	5d                   	pop    %rbp
 acb:	c3                   	retq   
