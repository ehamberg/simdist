/********************************************************************
 *   		master.h
 *   Created on Mon Feb 27 2006 by Boye A. Hoeverstad.
 *   Copyright 2009 Boye A. Hoeverstad
 *
 *   This file is part of Simdist.
 *
 *   Simdist is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Simdist is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Simdist.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   ***************************************************************
 *   
 *   This file contains the interface to the master part of the
 *   system.
 *******************************************************************/

#if !defined(__MASTER_H__)
#define __MASTER_H__

#include "slave.h"
#include "jobqueue.h"

// extern FeedbackError E_MASTER_EVALUATE;
DECLARE_FEEDBACK_ERROR(E_MASTER_EVALUATE)
// extern FeedbackError E_MASTER_EVALAGAIN;
DECLARE_FEEDBACK_ERROR(E_MASTER_EVALAGAIN)
// extern FeedbackError E_DISTRIBUTOR_CREATESLAVES;
DECLARE_FEEDBACK_ERROR(E_DISTRIBUTOR_CREATESLAVES)
// extern FeedbackError E_DISTRIBUTOR_DESTROY;
DECLARE_FEEDBACK_ERROR(E_DISTRIBUTOR_DESTROY)

/********************************************************************
 *   Evaluation distribution master.  This class fires up the slaves,
 *   and then evaluates a set of problems by distributing them to the
 *   slaves.  This happens by posting the jobs to the jobqueue, and
 *   then waiting for it to empty.
 *******************************************************************/
class Master
{
  JobQueue *m_pJobQueue;
  Feedback m_fb;

  int m_nIDCounter;
public:
  Master(JobQueue *pJobQueue);
  int Evaluate(const std::vector<std::string> &data, std::vector<std::string> &results);
};

/********************************************************************
 *   This class launches the system, by creating slaves, a queue and a
 *   master.  On successful completion, it will return a pointer to a
 *   newly created Master object.  Singleton.
 *******************************************************************/
class DistributorLauncher
{
  typedef struct TDistributionVar
  {
    std::vector<SlaveClient*> slaves;
    JobQueue *pJobQueue;
    std::vector<std::string> servers;
    std::vector<std::string> slavePrograms;
    std::vector<std::string> slavesArgs;
    ThreadBarrier *pBarrier;
  } TDistributor;

  typedef std::map<Master*, TDistributor> TDistributorSet;
  TDistributorSet m_distributors;

  Feedback m_fb;

  DistributorLauncher();
  ~DistributorLauncher();
public:
  static DistributorLauncher& Instance();
  int CreateDistributor(const std::vector<std::string> &servers, 
                        const std::vector<std::string> &slavePrograms,
                        const std::vector<std::string> &slavesArgs,
                        Master **ppNewMaster);
  int CreateDistributor(const std::vector<std::string> &servers, 
                        const std::string &sSlaveProgram,
                        const std::string &slaveArgs,
                        Master **ppNewMaster);
  int CreateDistributor(Master **ppNewMaster, int nNumSlaves);

  int DestroyDistributor(Master *pMaster);
};

int
GetChild(Feedback &fb, const std::string &sSide, std::string &sProgram, std::string &sArguments);

#endif
