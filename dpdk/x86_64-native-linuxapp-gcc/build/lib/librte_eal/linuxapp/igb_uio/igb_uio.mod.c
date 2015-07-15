#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xb89a34a1, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x5d41c87c, __VMLINUX_SYMBOL_STR(param_ops_charp) },
	{ 0x8b190a16, __VMLINUX_SYMBOL_STR(pci_unregister_driver) },
	{ 0x9af4f94d, __VMLINUX_SYMBOL_STR(__pci_register_driver) },
	{ 0xe2d5255a, __VMLINUX_SYMBOL_STR(strcmp) },
	{ 0x6ca1ddd4, __VMLINUX_SYMBOL_STR(__dynamic_dev_dbg) },
	{ 0xd0474d35, __VMLINUX_SYMBOL_STR(_dev_info) },
	{ 0xfc8c54cb, __VMLINUX_SYMBOL_STR(dev_notice) },
	{ 0xeeead00a, __VMLINUX_SYMBOL_STR(pci_intx_mask_supported) },
	{ 0xd1f18b7f, __VMLINUX_SYMBOL_STR(__uio_register_device) },
	{ 0x9cf067da, __VMLINUX_SYMBOL_STR(sysfs_create_group) },
	{ 0x54e1597b, __VMLINUX_SYMBOL_STR(pci_enable_msix) },
	{ 0xcfc62f27, __VMLINUX_SYMBOL_STR(xen_start_info) },
	{ 0x731dba7a, __VMLINUX_SYMBOL_STR(xen_domain_type) },
	{ 0x90206ede, __VMLINUX_SYMBOL_STR(dma_supported) },
	{ 0x42c8de35, __VMLINUX_SYMBOL_STR(ioremap_nocache) },
	{ 0x769dc43f, __VMLINUX_SYMBOL_STR(dma_set_mask) },
	{ 0xf8e67854, __VMLINUX_SYMBOL_STR(pci_set_master) },
	{ 0x93248fef, __VMLINUX_SYMBOL_STR(dev_err) },
	{ 0xd8a22f2e, __VMLINUX_SYMBOL_STR(pci_request_regions) },
	{ 0xde05cf1d, __VMLINUX_SYMBOL_STR(pci_enable_device) },
	{ 0x3b88f3a0, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0x16e17547, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0x76c19cce, __VMLINUX_SYMBOL_STR(pci_check_and_mask_intx) },
	{ 0x956f11a3, __VMLINUX_SYMBOL_STR(pci_intx) },
	{ 0x5113ca0e, __VMLINUX_SYMBOL_STR(pci_cfg_access_unlock) },
	{ 0xe1ccee8b, __VMLINUX_SYMBOL_STR(pci_cfg_access_lock) },
	{ 0x34b19b2c, __VMLINUX_SYMBOL_STR(remap_pfn_range) },
	{ 0xc715d9e0, __VMLINUX_SYMBOL_STR(boot_cpu_data) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xc92c66ea, __VMLINUX_SYMBOL_STR(pci_disable_msix) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0xb4549cdc, __VMLINUX_SYMBOL_STR(pci_disable_device) },
	{ 0x7082ffee, __VMLINUX_SYMBOL_STR(pci_release_regions) },
	{ 0xf15ac360, __VMLINUX_SYMBOL_STR(uio_unregister_device) },
	{ 0x18851c14, __VMLINUX_SYMBOL_STR(sysfs_remove_group) },
	{ 0xedc03953, __VMLINUX_SYMBOL_STR(iounmap) },
	{ 0x28318305, __VMLINUX_SYMBOL_STR(snprintf) },
	{ 0x4b97c8c3, __VMLINUX_SYMBOL_STR(pci_enable_sriov) },
	{ 0xe8e4088f, __VMLINUX_SYMBOL_STR(pci_num_vf) },
	{ 0xfb374c50, __VMLINUX_SYMBOL_STR(pci_disable_sriov) },
	{ 0x3c80c06c, __VMLINUX_SYMBOL_STR(kstrtoull) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=uio";


MODULE_INFO(srcversion, "1615B72D1C74E5105B464A1");
