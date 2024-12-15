set Xcnt 3
set Ycnt 1

set cr_area_region {}
set list_type {*RAMB18* *DSPs* *CLB\ Registers* *CLB\ LUTs*}
set list_arr_type {BRAM DSP FF LUT}

for {set i 0} {$i<$Xcnt} {incr i} {
    for {set j 0} {$j<$Ycnt} {incr j} {     
        
        set getPblock test_pblock_X${i}Y${j}

        create_pblock $getPblock
        resize_pblock $getPblock -add CLOCKREGION_X${i}Y${j}:CLOCKREGION_X${i}Y${j}

        set parse [split [report_utilization -pblocks $getPblock -return_string] "\n"]

        foreach type $list_type write_type $list_arr_type {
            set store [lindex [lsearch -all -inline $parse $type] 0]
            set splitStore [split $store "|"]
            set used [regsub -all { } [lindex $splitStore 5] ""]
            set available [regsub -all { } [lindex $splitStore 8] ""]

            lappend cr_area_region CR_AREA_\[${i}\]\[${j}\]\[\'$write_type\'\]\ =\ [expr $available - $used]
            #puts ${used}/$available

        }
    
        set store [lindex [lsearch -all -inline $parse *URAM*] 0]
        set length [llength [lsearch -all -inline $parse *URAM*]]
        if { $length > 1} {
            set splitStore [split $store "|"]
            set used [regsub -all { } [lindex $splitStore 5] ""]
            set available [regsub -all { } [lindex $splitStore 8] ""]

            lappend cr_area_region CR_AREA_\[${i}\]\[${j}\]\[\'URAM\'\]\ =\ [expr $available - $used]
            #puts ${used}/$available
        } else {
            lappend cr_area_region CR_AREA_\[${i}\]\[${j}\]\[\'URAM\'\]\ =\ 0
            #puts ${used}/$available
        }
        delete_pblocks $getPblock
    }
    puts $cr_area_region
}

set writeFile [open "dump.txt" w+]
foreach getVal $cr_area_region {
    puts $writeFile ${getVal}
}

close $writeFile