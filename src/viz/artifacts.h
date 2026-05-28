#pragma once

#include "refract/schema_registry.h"
#include "referee/referee.h"
#include "referee_sqlite/sqlite_store.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace iris::viz {

constexpr referee::TypeID kTypeVizPanel{0x56495A0000000001ULL};
constexpr referee::TypeID kTypeVizTextLog{0x56495A0000000002ULL};
constexpr referee::TypeID kTypeVizMetric{0x56495A0000000003ULL};
constexpr referee::TypeID kTypeVizTable{0x56495A0000000004ULL};
constexpr referee::TypeID kTypeVizTree{0x56495A0000000005ULL};
constexpr referee::TypeID kTypeVizTaskView{0x56495A0000000006ULL};

struct Panel {
  std::string title;
};

struct TextLog {
  std::vector<std::string> lines;
};

struct Metric {
  std::string name;
  double value{0.0};
};

struct Table {
  std::vector<std::string> columns;
  std::vector<std::vector<std::string>> rows;
};

struct Tree {
  std::string label;
  std::vector<referee::ObjectID> children;
};

struct TaskView {
  std::uint64_t task_id{0};
  std::string state;
  std::optional<referee::ObjectID> task_object;
  std::vector<referee::ObjectID> artifacts;
  std::vector<referee::ObjectID> diagnostics;
};

referee::Result<referee::ObjectID> create_panel(iris::refract::SchemaRegistry& registry,
                                                referee::SqliteStore& store,
                                                const Panel& panel);
referee::Result<referee::ObjectID> create_text_log(iris::refract::SchemaRegistry& registry,
                                                   referee::SqliteStore& store,
                                                   const TextLog& log);
referee::Result<referee::ObjectID> create_metric(iris::refract::SchemaRegistry& registry,
                                                 referee::SqliteStore& store,
                                                 const Metric& metric);
referee::Result<referee::ObjectID> create_table(iris::refract::SchemaRegistry& registry,
                                                referee::SqliteStore& store,
                                                const Table& table);
referee::Result<referee::ObjectID> create_tree(iris::refract::SchemaRegistry& registry,
                                               referee::SqliteStore& store,
                                               const Tree& tree);
referee::Result<referee::ObjectID> create_task_view(iris::refract::SchemaRegistry& registry,
                                                    referee::SqliteStore& store,
                                                    const TaskView& task);

} // namespace iris::viz
