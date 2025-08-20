## Build directory is injected by CMake at configure time.
set builddir "@CMAKE_BINARY_DIR@"
report_qor_suggestions -name qor_suggestions -max_paths 100 -max_strategies 3
report_qor_suggestions -of_objects [get_qor_suggestions]
file mkdir [file join $builddir qor_suggestions]
write_qor_suggestions -of_objects [get_qor_suggestions] -file [file join $builddir qor_suggestions rqs_report.rqs] -strategy_dir [file join $builddir qor_suggestions] -force

file mkdir [file join $builddir testDird]
read_qor_suggestion [file join $builddir qor_suggestions rqs_report.rqs]
#delete_qor_suggestions [get_qor_suggestions -filter {CATEGORY==Clocking}]
#report_qor_suggestions -of_objects [get_qor_suggestions]

place_design -unplace
place_design
