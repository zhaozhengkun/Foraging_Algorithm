#!/bin/sh

# Number of times to run ARGoS with different random seeds
runs=10

# Path to original ARGoS configuration file
config="foraging.argos"

# Create a temporary folder to store the modified ARGoS configuration files
mkdir temp

# Run ARGoS with the random seeds from 1 to the number of runs
for random_seed in $(seq 1 $runs)
do
	# Create a copy of the ARGoS configuration file, so that it can be safely modified
	temp="temp/foraging_${random_seed}.argos"
	cp $config $temp

	# Find and replace values in ARGoS configuration file
	sed -i "s/random_seed=\"[0-9]*\"/random_seed=\"$random_seed\"/" $temp
	sed -i "s/output_filename=\"[a-zA-Z0-9]*.csv\"/output_filename=\"output_${random_seed}.csv\"/" $temp

	# Run ARGoS with the modified configuration file
	argos3 -c $temp
done

# Delete the temporary folder
rm -rf temp