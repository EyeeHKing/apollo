/******************************************************************************
 * Copyright 2018 The Apollo Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

#ifndef CYBERTRON_SCHEDULER_POLICY_PROCESSOR_CONTEXT_H_
#define CYBERTRON_SCHEDULER_POLICY_PROCESSOR_CONTEXT_H_

#include <future>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "cybertron/base/atomic_hash_map.h"
#include "cybertron/base/atomic_rw_lock.h"
#include "cybertron/croutine/croutine.h"

namespace apollo {
namespace cybertron {
namespace scheduler {

using apollo::cybertron::base::AtomicHashMap;
using apollo::cybertron::base::AtomicRWLock;
using apollo::cybertron::base::ReadLockGuard;
using apollo::cybertron::base::WriteLockGuard;
using croutine::CRoutine;
using croutine::RoutineState;
using CRoutineList = std::list<std::shared_ptr<CRoutine>>;
using CRoutineMap = std::unordered_map<uint64_t, std::shared_ptr<CRoutine>>;

class Processor;
class ProcessorStat;

struct CommonState {
  bool running = false;
};

class ProcessorContext {
 public:
  ProcessorContext() : notified_(false) {}

  void ShutDown();

  inline void set_id(int id) { proc_index_ = id; }
  inline int id() const { return proc_index_; }
  inline bool Getstate(const uint64_t& routine_id, RoutineState* state);
  inline bool set_state(const uint64_t& routine_id, const RoutineState& state);

  void BindProcessor(const std::shared_ptr<Processor>& processor) {
    if (!processor_) {
      processor_ = processor;
    }
  }

  virtual void Notify(uint64_t croutine_id);
  virtual bool Enqueue(const std::shared_ptr<CRoutine>& cr) = 0;
  void RemoveCRoutine(uint64_t croutine_id);
  int RqSize();

  virtual bool RqEmpty() = 0;
  virtual std::shared_ptr<CRoutine> NextRoutine() = 0;

 protected:
  AtomicRWLock rw_lock_;
  CRoutineMap cr_map_;
  std::shared_ptr<Processor> processor_ = nullptr;

  bool stop_ = false;
  std::atomic<bool> notified_;
  uint32_t index_ = 0;
  uint32_t status_;
  int proc_index_ = -1;
};

bool ProcessorContext::Getstate(const uint64_t& routine_id,
                                RoutineState* state) {
  ReadLockGuard<AtomicRWLock> lg(rw_lock_);
  auto it = cr_map_.find(routine_id);
  if (it != cr_map_.end()) {
    *state = it->second->state();
    return true;
  }
  return false;
}

bool ProcessorContext::set_state(const uint64_t& routine_id,
                                 const RoutineState& state) {
  ReadLockGuard<AtomicRWLock> lg(rw_lock_);
  auto it = cr_map_.find(routine_id);
  if (it != cr_map_.end()) {
    it->second->set_state(state);
    return true;
  }
  return false;
}

}  // namespace scheduler
}  // namespace cybertron
}  // namespace apollo

#endif  // CYBERTRON_SCHEDULER_POLICY_PROCESSOR_CONTEXT_H_
