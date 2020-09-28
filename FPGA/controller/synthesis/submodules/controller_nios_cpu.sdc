# Legal Notice: (C)2020 Altera Corporation. All rights reserved.  Your
# use of Altera Corporation's design tools, logic functions and other
# software and tools, and its AMPP partner logic functions, and any
# output files any of the foregoing (including device programming or
# simulation files), and any associated documentation or information are
# expressly subject to the terms and conditions of the Altera Program
# License Subscription Agreement or other applicable license agreement,
# including, without limitation, that your use is for the sole purpose
# of programming logic devices manufactured by Altera and sold by Altera
# or its authorized distributors.  Please refer to the applicable
# agreement for further details.

#**************************************************************
# Timequest JTAG clock definition
#   Uncommenting the following lines will define the JTAG
#   clock in TimeQuest Timing Analyzer
#**************************************************************

#create_clock -period 10MHz {altera_reserved_tck}
#set_clock_groups -asynchronous -group {altera_reserved_tck}

#**************************************************************
# Set TCL Path Variables 
#**************************************************************

set 	controller_nios_cpu 	controller_nios_cpu:*
set 	controller_nios_cpu_oci 	controller_nios_cpu_nios2_oci:the_controller_nios_cpu_nios2_oci
set 	controller_nios_cpu_oci_break 	controller_nios_cpu_nios2_oci_break:the_controller_nios_cpu_nios2_oci_break
set 	controller_nios_cpu_ocimem 	controller_nios_cpu_nios2_ocimem:the_controller_nios_cpu_nios2_ocimem
set 	controller_nios_cpu_oci_debug 	controller_nios_cpu_nios2_oci_debug:the_controller_nios_cpu_nios2_oci_debug
set 	controller_nios_cpu_wrapper 	controller_nios_cpu_debug_slave_wrapper:the_controller_nios_cpu_debug_slave_wrapper
set 	controller_nios_cpu_jtag_tck 	controller_nios_cpu_debug_slave_tck:the_controller_nios_cpu_debug_slave_tck
set 	controller_nios_cpu_jtag_sysclk 	controller_nios_cpu_debug_slave_sysclk:the_controller_nios_cpu_debug_slave_sysclk
set 	controller_nios_cpu_oci_path 	 [format "%s|%s" $controller_nios_cpu $controller_nios_cpu_oci]
set 	controller_nios_cpu_oci_break_path 	 [format "%s|%s" $controller_nios_cpu_oci_path $controller_nios_cpu_oci_break]
set 	controller_nios_cpu_ocimem_path 	 [format "%s|%s" $controller_nios_cpu_oci_path $controller_nios_cpu_ocimem]
set 	controller_nios_cpu_oci_debug_path 	 [format "%s|%s" $controller_nios_cpu_oci_path $controller_nios_cpu_oci_debug]
set 	controller_nios_cpu_jtag_tck_path 	 [format "%s|%s|%s" $controller_nios_cpu_oci_path $controller_nios_cpu_wrapper $controller_nios_cpu_jtag_tck]
set 	controller_nios_cpu_jtag_sysclk_path 	 [format "%s|%s|%s" $controller_nios_cpu_oci_path $controller_nios_cpu_wrapper $controller_nios_cpu_jtag_sysclk]
set 	controller_nios_cpu_jtag_sr 	 [format "%s|*sr" $controller_nios_cpu_jtag_tck_path]

#**************************************************************
# Set False Paths
#**************************************************************

set_false_path -from [get_keepers *$controller_nios_cpu_oci_break_path|break_readreg*] -to [get_keepers *$controller_nios_cpu_jtag_sr*]
set_false_path -from [get_keepers *$controller_nios_cpu_oci_debug_path|*resetlatch]     -to [get_keepers *$controller_nios_cpu_jtag_sr[33]]
set_false_path -from [get_keepers *$controller_nios_cpu_oci_debug_path|monitor_ready]  -to [get_keepers *$controller_nios_cpu_jtag_sr[0]]
set_false_path -from [get_keepers *$controller_nios_cpu_oci_debug_path|monitor_error]  -to [get_keepers *$controller_nios_cpu_jtag_sr[34]]
set_false_path -from [get_keepers *$controller_nios_cpu_ocimem_path|*MonDReg*] -to [get_keepers *$controller_nios_cpu_jtag_sr*]
set_false_path -from *$controller_nios_cpu_jtag_sr*    -to *$controller_nios_cpu_jtag_sysclk_path|*jdo*
set_false_path -from sld_hub:*|irf_reg* -to *$controller_nios_cpu_jtag_sysclk_path|ir*
set_false_path -from sld_hub:*|sld_shadow_jsm:shadow_jsm|state[1] -to *$controller_nios_cpu_oci_debug_path|monitor_go