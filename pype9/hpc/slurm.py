#!/usr/bin/env python
"""
  Wraps a executable to enable that script to be submitted to an SGE cluster
  engine

  Author: Thomas G. Close (tclose@oist.jp)
  Copyright: 2012-2014 Thomas G. Close.
  License: This file is part of the "NineLine" package, which is released under
           the MIT Licence, see LICENSE for details.
"""
import os.path
import subprocess
from .base import HPCSubmitter


class SlurmSubmitter(HPCSubmitter):
    """
    This class automates the submission of MPI jobs on a SGE-powered high-
    performance computing cluster
    """

    def _write_jobscript(self, jobscript_path, args, env, name_cmd,
                         cmdline, copy_cmd, time_limit_option):
        """
        Create a jobscript in the work directory and then submit it to the
        tombo que

        `override_args`  -- Override existing arguments parsed from command
                            line
        `env`            -- The required environment variables (defaults to
                            those generated by '_create_env(work_dir)')
        `copy_to_output` -- Directories to copy into the output directory
        `strip_build_from_copy` -- Removes all files and directories to be
                            copied that have the name 'build'
        `name`           -- Records a name for the run (when generating
                            multiple runs) so that the output directory can be
                            easily renamed to a more informative name after it
                            is copied to its destination via the command "mv
                            <output_dir> `cat <output_dir>/name`"
        """
        with open(jobscript_path, 'w') as f:
            f.write(
                """#!/usr/bin/env sh

# Parse the job script using this shell
#$ -S /bin/bash

# Send stdout and stderr to the same file.
#$ -j y

# Standard output and standard error files
#$ -o {logging_path}
#$ -e {logging_path}

# Name of the queue
#$ -q {que_name}

# Setup mailing list
{email}
#$ -m abe

# use OpenMPI parallel environment with {np} processes
#$ -pe openmpi {np}
{time_limit}
# Set the memory limits for the script
#$ -l h_vmem={max_memory}
#$ -l virtual_free={mean_memory}

# Export the following env variables:
#$ -v HOME
#$ -v PATH
#$ -v PYTHONPATH
#$ -v LD_LIBRARY_PATH
#$ -v NINEMLP_SRC_PATH
#$ -v NINEMLP_MPI
#$ -v BREP_DEVEL
#$ -v PARAMDIR
#$ -v VERBOSE

###################################################
### Copy the model to all machines we are using ###
###################################################

# Set up the correct paths
export PATH={path}:$PATH
export PYTHONPATH={pythonpath}
export LD_LIBRARY_PATH={ld_library_path}

echo "============== Starting mpirun ==============="

cd {work_dir}
time mpirun {cmdline}

echo "============== Mpirun has ended =============="

echo "Copying files to output directory '{output_dir}'"
cp -r {work_dir}/output {output_dir}
cp {jobscript_path} {output_dir}/job
{name_cmd}
{copy_cmd}

echo "============== Done ==============="

cp {logging_path} {output_dir}/log
"""
                .format(work_dir=self.work_dir, args=args,
                        path=env.get('PATH', ''), np=self.num_processes,
                        que_name=self.que_name, max_memory=self.max_memory,
                        mean_memory=self.mean_memory,
                        pythonpath=env.get('PYTHONPATH', ''),
                        ld_library_path=env.get('LD_LIBRARY_PATH', ''),
                        cmdline=cmdline, output_dir=self.output_dir,
                        name_cmd=name_cmd, copy_cmd=copy_cmd,
                        jobscript_path=jobscript_path,
                        time_limit=time_limit_option,
                        email=('#$ -M ' + os.environ['EMAIL']
                               if 'EMAIL' in os.environ else ''),
                        logging_path=self.logging_path))

    def _submit_to_que(self, jobscript_path):
        sub_text = subprocess.check_output(
            'qsub {}'.format(jobscript_path),
            shell=True)
        print sub_text
        # Save jobID to file inside output directory
        with open(os.path.join(self.work_dir,
                               'output', 'jobID'), 'w') as f:
            f.write(sub_text.split()[2])
