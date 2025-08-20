## Vivado log path is injected by CMake at configure time.
set VIVADO_LOG "@VIVADO_LOG_PATH@"

set NLOOPS 5 
set TNS_PREV 0
set WNS_SRCH_STR "WNS="
set TNS_SRCH_STR "TNS="

set WNS [ exec grep $WNS_SRCH_STR $VIVADO_LOG | tail -1 | sed -n -e "s/^.*$WNS_SRCH_STR//p" | cut -d\  -f 1] 

if {$WNS < 0.000} {
    
    for {set i 0} {$i < $NLOOPS} {incr i} {
        phys_opt_design -directive AggressiveExplore
        # get WNS / TNS by getting lines with the search string in it (grep),
        # get the last line only (tail -1),
        # extracting everything after the search string (sed), and
        # cutting just the first value out (cut). whew!
        set WNS [ exec grep $WNS_SRCH_STR $VIVADO_LOG | tail -1 | sed -n -e "s/^.*$WNS_SRCH_STR//p" | cut -d\  -f 1]                                    
        set TNS [ exec grep $TNS_SRCH_STR $VIVADO_LOG | tail -1 | sed -n -e "s/^.*$TNS_SRCH_STR//p" | cut -d\  -f 1]
        if {($TNS == $TNS_PREV && $i > 0) || $WNS >= 0.000} {
            break
        }
        set TNS_PREV $TNS

        phys_opt_design -directive AggressiveFanoutOpt 
        set WNS [ exec grep $WNS_SRCH_STR $VIVADO_LOG | tail -1 | sed -n -e "s/^.*$WNS_SRCH_STR//p" | cut -d\  -f 1]
        set TNS [ exec grep $TNS_SRCH_STR $VIVADO_LOG | tail -1 | sed -n -e "s/^.*$TNS_SRCH_STR//p" | cut -d\  -f 1]
        if {($TNS == $TNS_PREV && $i > 0) || $WNS >= 0.000} {
            break
        }
        set TNS_PREV $TNS

        phys_opt_design -directive AlternateReplication
        set WNS [ exec grep $WNS_SRCH_STR $VIVADO_LOG | tail -1 | sed -n -e "s/^.*$WNS_SRCH_STR//p" | cut -d\  -f 1]
        set TNS [ exec grep $TNS_SRCH_STR $VIVADO_LOG | tail -1 | sed -n -e "s/^.*$TNS_SRCH_STR//p" | cut -d\  -f 1]
        if {($TNS == $TNS_PREV) || $WNS >= 0.000} {
            break
        }
        set TNS_PREV $TNS
    }

    report_timing_summary -file level0_wrapper_phys_opt_loop.rpt
    report_design_analysis -logic_level_distribution -of_timing_paths [get_timing_paths -max_paths 10000 -slack_lesser_than 0] -file level0_wrapper_phys_opt_loop_violation.rpt
    write_checkpoint -force level0_wrapper_phys_opt_loop.dcp
}