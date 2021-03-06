# DNA-Nanoscope
DNA Nanoscope "sequencing by imaging" reconstruction scripts

### 1. Demultiplex reads from different experiments:
The ends of the reads are scanned for sequencing barcodes (using local sequence alignment) and sorted into subdirectories based on barcodes identified. The MATLAB command used was:

```
sort_barcoded_reads(fastq_dir,ONT_barcodes)
```

where:\
```fastq_dir``` is the path of the fastq sequence files and\
```ONT_barcodes``` specifies the sequencing barcodes and their reverse complements.\
The output is written into fastq files with reads that are sorted into sub-directories corresponding to the identity of the sequencing barcodes.

### 2. Extract record lengths and assign to correct target pair: 
Reads are scanned to identify the unique staple barcode sequences associated with each distance record. The length of the repeat region between the barcode sequences is extracted and assigned to the appropriate target pair. The following MATLAB command was executed for each directory containing reads from the same experiment:

```
pairwise_record_list = extract_pairwise_record_lengths(target_barcodes,path, library_size, color_length)
```

where:\
```target_barcodes``` specifies the staple barcode sequences,\
```path``` specifies the path of the fastq reads,\
```library_size``` specifies the number of sequencing libraries that were combined for a single run,\
```color_length``` specifies the length of the auxiliary tag sequence, and\
```pairwise_record_list``` is the output, a matrix of size (n, n, 2001) where n is the number of target points and cell (i, j, k) holds the number of distance records of length k bases (only counting the repeat region) between points i and j. All distance records of length > 2000 are stored in the slice (:, :, 2001).\
```pairwise_record_list``` variables from different sequencing runs of the same experiment were combined by simply adding them.

### 3. Infer the measured distance between every target pair: 
The distribution of distance record lengths between every pair of points was examined to identify the major peak (in base pairs), which was then converted into a distance (in nanometers) by applying the calibration function. The MATLAB function used was:

```
[pairwise_distances, pairwise_peak_heights] = finddist_geometry(pairwise_records_list,calibration_fun)
```

where:\
pairwise_records_list is the output of extract_pairwise_record_lengths (see previous step),\
calibration_fun is a cfit object that holds the calibration function of the ruler, which maps bases to nanometers,\
pairwise_distances is one output, an array of measured distances for each pair of points, and\
pairwise_peak_heights is the peak height (in bases) corresponding to the distances measured. It is a measure of the confidence in the measurement.

### 4. Reconstruct geometry from pairwise distance measurements:
The distance measurements are integrated into a coherent embedding of the targets in the 2D Euclidean plane, using the following MATLAB function:

```
[theta, prune, score] = solveDGP(pairwise_distances, pairwise_peak_heights, opt_threshold)
```

where:\
```pairwise_distances``` and ```pairwise_peak_heights``` are the outputs from ```finddist_geometry``` (see previous step),\
```opt_threshold``` is a parameter used to prune unreliable measurements and to generate weights for measurements reflecting their reliability (see supplementary text for details on how opt_threshold is auto-set),\
```theta``` is a list of coordinates, specifying the final embedding,\
```prune``` is a logical-valued array indicating which target points were dropped from the final reconstruction, and\
```score``` is a measure of the internal consistency of the embedding. See supplementary text S2D.\
 The final embedding is compared to the designed pattern by superimposing them to minimize the RMSD (root-mean-square deviation). The MATLAB script used is:

```
[theta_translated, lrms] = superimpose(theta, theta_designed, prune)
```

where:\
```theta``` and ```prune``` are the outputs from ```solveDGP```,\
```theta_designed``` contains a list of coordinates specifying the designed pattern,\
```theta_translated``` is the superimposition of the final embedding that minimizes the RMSD between the designed and reconstructed pattern, and\
```lrms``` is the corresponding RMSD.