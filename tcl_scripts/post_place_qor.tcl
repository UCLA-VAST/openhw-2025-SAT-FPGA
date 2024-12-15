set dirname "/scratch/milo168/micro2024_8"
report_qor_suggestions -name qor_suggestions -max_paths 100 -max_strategies 3
report_qor_suggestions -of_objects [get_qor_suggestions]
write_qor_suggestions -of_objects [get_qor_suggestions] -file $dirname/qor_suggestions/rqs_report.rqs -strategy_dir $dirname/qor_suggestions -force

file mkdir $dirname/testDird
read_qor_suggestion $dirname/qor_suggestions/rqs_report.rqs
#delete_qor_suggestions [get_qor_suggestions -filter {CATEGORY==Clocking}]
#report_qor_suggestions -of_objects [get_qor_suggestions]

place_design -unplace
place_design
