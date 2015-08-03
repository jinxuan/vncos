Mini-OS是跟随Xen hypervisor发布的一款小内核操作系统，目前有一些基于Mini-OS的应用：HP实验室在2007年发布了基于Mini-OS的Xen Library OS；2008年剑桥大学进一步完善了HP的工作，完成了HVM stubdom并置于Xen源码树中；伊利诺伊大学芝加哥分校(UIC)正致力于开发一款基于Mini-OS的小内核系统—Ethos。



# Mini-OS 启动简介 #

Mini-OS在\*start**处启动，加载SS和ESP指向的地址。KERNEL\_SS由Xen的GDT提供，ESP指向的地址取自于\*stack\_start** 。ESI寄存器指向Mini-OS的\*start\_info\_t**结构体。ESI做为参数传入到启动函数\*start\_kerel()** 中。

**start\_kerel()** 是启动Mini-OS的例程。它调用了一些初始化函数，然后建立了三个内核线程。由于Mini-OS是不可抢占的并只能运行单线程的操作系统，所以这三个内核线程依次被创建。

## arch\_init() ##

  * 将start\_info数据结构拷贝到内核映像的一片全局区中(start\_info\_union.start\_info)
  * 将全局变量phys\_to\_machine\_mapping(mm.c)指向现有的P2M表(start\_info.mfn\_list)
  * 使用hypercall：HYPERVISOR\_update\_va\_mapping把shared\_info页映射到0x2000
  * 注册回调句柄(callback handlers)
```
// 通过set_callbacks()，注册各种event和failsafe的处理函数
set_callbacks(unsigned long event_selector, unsigned long event_address, unsigned long failsafe_selector, unsigned long failsafe_address)
```

## trap\_init() ##

注册陷阱表(trap handler table)，通常定义于Mini-OS的arch/x86/x86\_32.h中。
```
void trap_init(void)
{
    HYPERVISOR_set_trap_table(trap_table);
}
```

## init\_mm() ##

该例程初始化Mini-OS的内存管理功能。

  * arch\_init\_mm()
> > 首先计算Mini-OS可以使用的页帧数。在Mini-OS中，text段起始于0x00，虚拟地址和客户物理地址是一致的。然后调用build\_pagetables(start\_pfg,max\_pfn)，建立页表。每增加一个页表项，调用HYPERVISOR\_mmu\_update()同步到hypervisor中。
  * arch\_init\_p2m()
> > 初始化HYPERVISOR\_shared\_info->arch.pfg\_to\_mfn\_frame\_list结构体，它记录了客户机页帧到物理机页帧的映射。
  * arch\_init\_demand\_mapping\_area()
> > 创建一个额外的PTE，可以用来按需地映射大于max\_pfn页地址。

# Xen Library OS #

## Xen Library OS的设计与实现 ##

Xen Library OS 是一个类虚拟化的操作系统，它由四部分组成：

  * 一个基于GNU工具链的cross-development环境
  * Red Hat的newlib C库
  * Mini-OS内核
  * 域间通信机制IDC，基于Xen共享内存和时间通道实现


> http://vncos.googlecode.com/svn/trunk/picture/overview_of_application_and_libraries.JPG

### 开发工具链 ###

加入一个基于GNU工具链的cross-development环境，目的是获得最大的代码兼容性。Linux上的进程可以兼容的运行在Library OS上。通过修改GNU编译器和二进制工具来支持新的目标体系结构。Library OS的编译器称作i386-minios-gcc，它的头文件、库和加载器都被重写。

### Library OS ###

Library OS使用Red Hat的newlib C库支持C程序。但需要修改newlib库以适应Library OS，比如一些函数fork、exec等都不能在新的目标体系结构中使用。
Library OS选择Mini-OS为其kernel，主要原因是Mini-OS有完善的支持Xen类虚拟化接口，Xen社区对其提供维护。
由于程序、库、内核都运行在一个地址空间中，所以Library OS适合运行小的可信服务，而不是一般的用户程序。

### 域间通信IDC ###

> http://vncos.googlecode.com/svn/trunk/picture/Inter-domain_communications_using_a_shared_ring.JPG

IDC是由Xen的事件通道实现的，包含读写两个IO环。IDC做为一个内核模块，向上层应用暴露6个接口。
  * Init:      初始化一个通道
  * Close:     关闭一个通道
  * Create:    允许用户向通道写数据，用户要有目标域的ID
  * Connect:   允许用户在通道中读数据
  * Write:     向通道中写
  * Read:      在通道中读

## Xen Library OS的应用 ##

> http://vncos.googlecode.com/svn/trunk/picture/vTPM_software_architecture_for_xen.JPG

Xen 3.0的vTPM驱动体系结构如图所示。所有的vTPM操作都有Domain0完成。

> http://vncos.googlecode.com/svn/trunk/picture/modified_vTPM_software_architecture.JPG

如图所示，vTPM的守护进程置于Domain0中，而vTPM的实际模拟器放入Library OS中，它们之间通过IDC联系。并且使用修改过的GNU工具链编译安装vTPM emulator。DomainU通过vTPM前段驱动将请求发往Domaoin0的后端驱动，vTPM的守护进程转发请求到Domain0的IDC，实际由Library OS处理请求。

修改Domain0的vTPM守护进程大概花费了200行代码，而修改vTPM emulator更少于500行代码。

该结构的缺点是显而易见的，由于每个DomU都配属相应的Trust Dom，使得一些操作变得非常繁琐，比如虚拟域热迁移等。

# 轻量级Xen Domain #

## 轻量级Xen Domain的设计与实现 ##

该轻量级Xen Domain 保留了Library OS的所有特性，包括cross-development环境、newlib库和IDC机制，并优化部分环境，更适合HPC(High Performance Computing)应用。

> http://vncos.googlecode.com/svn/trunk/picture/structure_of_our_lightweight_guest_domain.JPG

如图所示，轻量级Xen Domain的设计思想是将Mini-OS内核、C库、IP堆栈、执行环境和单个应用程序有机的整合在一起。在技术方面看，需要安装一个plain ELF cross-compilation chain，即以plain ELF为目标建立可交叉编译的binutils和gcc。然后用交叉编译工具编译和安装各种C库，它们和Mini-OS有同样的内核C flag。

LwIP提供了一个轻量级的TCP/IP协议栈，它与Mini-OS中的Network frontend相连接。Newlib提供了一个标准C库，它比庞大的GNU libc更适用于轻量级环境。但需要修改部分newlib的系统调用：

  * Mini-OS没有传统UNIX进程概念，getpid及其相似函数只输出某特定值，比如1
  * Mini-OS没有使用信号，可以忽略各种sig函数
  * 适当修改的函数：
    1. Sleep和gettimeofday必须小心的设计，使其符合虚拟化应用
    1. Mmap只负责处理匿名内存(anonymous memory)，而不用实现映射普通文件和为无关联的进程提供共享空间的功能。

该系统还实现了一个瘦虚拟文件层(thin virtual file layer)，它对控制台、文件、块设备、网络设备以及TCP/IP协议函数提供统一的支持。实现思想是依靠文件描述符识别目标文件的类型，然后将操作重定向到Mini-OS的各个前端驱动或者内核模块的底层函数上。

轻量级Xen Domain的进程调度完全依靠Mini-OS实现。Mini-OS提供非抢占式的多线程，并且目前只支持单虚拟CPU。这些线程由一个运行队列维护，它们之间没有优先权的差异。Mini-OS的内核也不需要支持任何自旋锁机制。
在内存管理方面借鉴了HPC(High Performance Computer)常用的sparse data机制，提供内存读写效率。

为提高读写磁盘的效率，应用了类似“零拷贝”的技术，IO操作不再由缓冲区缓冲。这意味着应用程序可以直接控制各种磁盘操作，不经过Mini-OS缓冲。

## 轻量级Xen Domain的应用—HVM stub domain ##

> http://vncos.googlecode.com/svn/trunk/picture/Stubdomain_approach.JPG

剑桥团队应用该轻量级Xen Domain将 qemu移到一个单独的驱动域中。由于Mini-OS调度非常简单并且只运行qemu一个程序，大大简化了hypervisor的调度和审计工作。在stubdom 中，qemu可以直接调用系统服务，不需要经过用户、内核两态的转换，提高了效率。

# Ethos - An Operating system for the Xen hypervisor #

Ethos 是UIC(University of Illinois at Chicago)正在开发的一款基于Xen和Mini-OS的轻量级操作系统。它开发的初衷是为了避免开发驱动程序、文件系统和网络设备。Ethos做为一个内核，只向外提供进程和系统调用接口。该项目的负责人Prof. Solworth称其为"Paired-OS"。

> http://vncos.googlecode.com/svn/trunk/picture/Ethos_structure.JPG

Xen上同时运行Ethos和标准linux。Ethos将文件系统、网络和设备驱动托管给linux，剩余部分由Ethos实现。

目前该项工作正在进展之中。

# 参考文献 #

1.Satya Popuri, A tour of the Mini-OS kernel, http://www.cs.uic.edu/~spopuri/minios.html

2.M.J. Anderson, M. Moffie, and C.I. Dalton. Towards Trustworthy Virtualisation Environments: Xen Library OS Security Service Infrastructure. Technical Report HPL-2007-69, Hewlett-Packard Development Company, L.P., April 2007.

3.Samuel Thibault and Tim Deegan. Improving performance by embedding hpc applications in lightweight xen domains. In HPCVirt '08: Proceedings of the 2nd workshop on Systemlevel virtualization for high performance computing, pages 9--15, New York, NY, USA, 2008. ACM.

4.Satya Popuri, Ethos - An Operating system for the Xen hypervisor, http://www.cs.uic.edu/~spopuri/ethos.html

5.Jon A. Solworth, Ethos: an operating system which creates a culture of security, http://www.rites.uic.edu/~solworth/ethos.html