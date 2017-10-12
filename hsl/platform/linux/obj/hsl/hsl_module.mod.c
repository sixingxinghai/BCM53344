#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.arch = MODULE_ARCH_INIT,
};

MODULE_INFO(intree, "Y");

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{        0, "release_sock" },
	{        0, "kmalloc_caches" },
	{        0, "__kmalloc" },
	{        0, "sock_init_data" },
	{        0, "__aeabi_idivmod" },
	{        0, "__copy_from_user" },
	{        0, "del_timer" },
	{        0, "strlen" },
	{        0, "sock_no_setsockopt" },
	{        0, "sock_no_getsockopt" },
	{        0, "get_random_bytes" },
	{        0, "sock_no_ioctl" },
	{        0, "skb_clone" },
	{        0, "__aeabi_uidivmod" },
	{        0, "sock_no_getname" },
	{        0, "remove_proc_entry" },
	{        0, "skb_recv_datagram" },
	{        0, "init_timer_key" },
	{        0, "sock_rfree" },
	{        0, "sprintf" },
	{        0, "jiffies" },
	{        0, "strcmp" },
	{        0, "__netdev_alloc_skb" },
	{        0, "sock_no_sendpage" },
	{        0, "sock_no_mmap" },
	{        0, "__aeabi_uidiv" },
	{        0, "__memzero" },
	{        0, "skb_queue_purge" },
	{        0, "memset" },
	{        0, "sock_no_socketpair" },
	{        0, "proc_mkdir" },
	{        0, "__sock_recv_wifi_status" },
	{        0, "sk_alloc" },
	{        0, "_ctype" },
	{        0, "printk" },
	{        0, "sock_no_bind" },
	{        0, "sscanf" },
	{        0, "memcmp" },
	{        0, "strncpy" },
	{        0, "strncmp" },
	{        0, "sock_no_listen" },
	{        0, "warn_slowpath_null" },
	{        0, "sock_no_accept" },
	{        0, "sk_free" },
	{        0, "add_timer" },
	{        0, "skb_pull" },
	{        0, "__aeabi_idiv" },
	{        0, "sock_no_shutdown" },
	{        0, "__sock_recv_timestamp" },
	{        0, "strcat" },
	{        0, "capable" },
	{        0, "kmem_cache_alloc" },
	{        0, "__alloc_skb" },
	{        0, "datagram_poll" },
	{        0, "sock_register" },
	{        0, "kfree_skb" },
	{        0, "create_proc_entry" },
	{        0, "skb_copy_datagram_iovec" },
	{        0, "_raw_spin_lock" },
	{        0, "sock_no_connect" },
	{        0, "_raw_write_unlock_bh" },
	{        0, "_raw_read_lock_bh" },
	{        0, "_raw_read_unlock_bh" },
	{        0, "kfree" },
	{        0, "memcpy" },
	{        0, "sock_unregister" },
	{        0, "memcpy_fromiovec" },
	{        0, "_raw_write_lock_bh" },
	{        0, "simple_strtol" },
	{        0, "snprintf" },
	{        0, "vsprintf" },
	{        0, "strcpy" },
	{        0, "skb_free_datagram" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

