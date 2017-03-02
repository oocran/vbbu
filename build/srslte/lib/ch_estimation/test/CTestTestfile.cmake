# CMake generated Testfile for 
# Source directory: /home/guillem/DADES/NLnet/v1/srsLTE/srslte/lib/ch_estimation/test
# Build directory: /home/guillem/DADES/NLnet/v1/srsLTE/build/srslte/lib/ch_estimation/test
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
ADD_TEST(chest_test_dl_cellid0 "chest_test_dl" "-c" "0" "-r" "50")
ADD_TEST(chest_test_dl_cellid1 "chest_test_dl" "-c" "1" "-r" "50")
ADD_TEST(chest_test_dl_cellid2 "chest_test_dl" "-c" "2" "-r" "50")
ADD_TEST(chest_test_ul_cellid0 "chest_test_ul" "-c" "0" "-r" "50")
ADD_TEST(chest_test_ul_cellid1 "chest_test_ul" "-c" "2" "-r" "50")
