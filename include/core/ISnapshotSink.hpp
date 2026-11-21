#pragma once

#include "models/SystemSnapshot.hpp"

// Receives a completed snapshot synchronously. Implementations must copy it
// if they need to retain data after consume returns.
class ISnapshotSink {
public:
    virtual ~ISnapshotSink() = default;
    virtual void consume(const SystemSnapshot& snapshot) = 0;
};
