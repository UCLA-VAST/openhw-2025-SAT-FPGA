set list_arr_type {BRAM DSP FF LUT}

set report [report_clock_utilization -return_string]
set cr_report [lindex [regexp -all -inline {5. Clock Regions : Load Primitives\n-(.*?)6. Clock Regions} $report] 1]
set split_cr_report [split $cr_report "\n"]
set parsedCR [lsearch -all -inline -regexp $split_cr_report {X[0-9]Y[0-9].*}]

set cr_area_region {}

foreach line $parsedCR {
    #puts $line
    set parseLine [split $line "|"]
    set xVal [lindex [regexp -all -inline {[0-9]*} [regsub -all { } [lindex $parseLine 1] ""]] 1]
    set yVal [lindex [regexp -all -inline {[0-9]*} [regsub -all { } [lindex $parseLine 1] ""]] 3]

    set usedFF [regsub -all { } [lindex $parseLine 4] ""]
    set availableFF [regsub -all { } [lindex $parseLine 5] ""]

    set usedLUT [regsub -all { } [lindex $parseLine 6] ""]
    set availableLUT [regsub -all { } [lindex $parseLine 7] ""]

    set usedBRAM [regsub -all { } [lindex $parseLine 8] ""]
    set availableBRAM [regsub -all { } [lindex $parseLine 9] ""]

    set usedURAM [regsub -all { } [lindex $parseLine 10] ""]
    set availableURAM [regsub -all { } [lindex $parseLine 11] ""]

    set usedDSP [regsub -all { } [lindex $parseLine 12] ""]
    set availableDSP [regsub -all { } [lindex $parseLine 13] ""]

    lappend cr_area_region CR_AREA_\[${xVal}\]\[${yVal}\]\[\'BRAM\'\]\ =\ [expr $availableBRAM - $usedBRAM]
    lappend cr_area_region CR_AREA_\[${xVal}\]\[${yVal}\]\[\'DSP\'\]\ =\ [expr $availableDSP - $usedDSP]
    lappend cr_area_region CR_AREA_\[${xVal}\]\[${yVal}\]\[\'FF\'\]\ =\ [expr $availableFF - $usedFF]
    lappend cr_area_region CR_AREA_\[${xVal}\]\[${yVal}\]\[\'LUT\'\]\ =\ [expr $availableLUT - $usedLUT]
    lappend cr_area_region CR_AREA_\[${xVal}\]\[${yVal}\]\[\'URAM\'\]\ =\ [expr $availableURAM - $usedURAM]
}

set writeFile [open "dump.txt" w+]
foreach getVal $cr_area_region {
    puts $writeFile ${getVal}
}

close $writeFile