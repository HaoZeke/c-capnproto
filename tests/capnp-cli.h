/* capnp-cli.h - helpers for tests that drive the official `capnp` tool.
 * Included by more than one test file; every function is static. */

#ifndef TESTS_CAPNP_CLI_H
#define TESTS_CAPNP_CLI_H

#include <gtest/gtest.h>

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include "capnp_c.h"

#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#ifndef CAPNP_CLI
#define CAPNP_CLI ""
#endif

#ifndef CAPNP_SOURCE_ROOT
#define CAPNP_SOURCE_ROOT ""
#endif

static capn_text chars_to_text(const char *chars) {
  return (capn_text) {
    .len = (int) strlen(chars),
    .str = chars,
    .seg = NULL,
  };
}

#define EXPECT_CAPN_TEXT_EQ(expected, t) \
  do { \
    EXPECT_EQ(strlen((expected)), (uint32_t) (t).len); \
    EXPECT_STREQ((expected), (t).str); \
  } while (0)

static const char *capnp_bin(void) {
  const char *env = getenv("CAPNP_CLI");
  if (env && env[0] != '\0') {
    return env;
  }
  if (CAPNP_CLI[0] != '\0') {
    return CAPNP_CLI;
  }
  return NULL;
}

static std::string source_root(void) {
  const char *env = getenv("CAPNP_SOURCE_ROOT");
  if (env && env[0] != '\0') {
    return env;
  }
  if (CAPNP_SOURCE_ROOT[0] != '\0') {
    return CAPNP_SOURCE_ROOT;
  }
  return ".";
}

static std::string join_path(const std::string &dir, const char *name) {
  if (dir.empty()) {
    return name;
  }
  char last = dir[dir.size() - 1];
  if (last == '/' || last == '\\') {
    return dir + name;
  }
  return dir + "/" + name;
}

static std::string include_dir(void) {
  return join_path(source_root(), "compiler");
}

static std::string addressbook_schema(void) {
  return join_path(source_root(), "tests/addressbook.capnp");
}

static std::string testalltypes_schema(void) {
  return join_path(source_root(), "compiler/test.capnp");
}

static std::string fixture_path(const char *name) {
  return join_path(source_root(), (std::string("tests/fixtures/") + name).c_str());
}

static bool capnp_present(void) {
  const char *bin = capnp_bin();
  return bin != NULL && access(bin, X_OK) == 0;
}

struct CmdResult {
  int status;
  std::string out;
  std::string err;
};

static void close_fd(int *fd) {
  if (*fd >= 0) {
    close(*fd);
    *fd = -1;
  }
}

static bool read_all_fd(int fd, std::string *out) {
  char buf[4096];
  ssize_t n;
  while ((n = read(fd, buf, sizeof(buf))) > 0) {
    out->append(buf, (size_t) n);
  }
  return n == 0;
}

static CmdResult run_argv(const std::vector<std::string> &argv,
                          const void *stdin_data, size_t stdin_len) {
  CmdResult r;
  r.status = -1;
  if (argv.empty()) {
    r.err = "empty argv";
    return r;
  }

  int in_pipe[2] = {-1, -1};
  int out_pipe[2] = {-1, -1};
  int err_pipe[2] = {-1, -1};
  if (pipe(in_pipe) != 0 || pipe(out_pipe) != 0 || pipe(err_pipe) != 0) {
    r.err = "pipe failed";
    close_fd(&in_pipe[0]);
    close_fd(&in_pipe[1]);
    close_fd(&out_pipe[0]);
    close_fd(&out_pipe[1]);
    close_fd(&err_pipe[0]);
    close_fd(&err_pipe[1]);
    return r;
  }

  pid_t pid = fork();
  if (pid < 0) {
    r.err = "fork failed";
    close_fd(&in_pipe[0]);
    close_fd(&in_pipe[1]);
    close_fd(&out_pipe[0]);
    close_fd(&out_pipe[1]);
    close_fd(&err_pipe[0]);
    close_fd(&err_pipe[1]);
    return r;
  }

  if (pid == 0) {
    if (dup2(in_pipe[0], STDIN_FILENO) < 0 ||
        dup2(out_pipe[1], STDOUT_FILENO) < 0 ||
        dup2(err_pipe[1], STDERR_FILENO) < 0) {
      _exit(127);
    }
    close(in_pipe[0]);
    close(in_pipe[1]);
    close(out_pipe[0]);
    close(out_pipe[1]);
    close(err_pipe[0]);
    close(err_pipe[1]);
    std::vector<char *> cargv;
    cargv.reserve(argv.size() + 1);
    for (size_t i = 0; i < argv.size(); i++) {
      cargv.push_back(const_cast<char *>(argv[i].c_str()));
    }
    cargv.push_back(NULL);
    execv(argv[0].c_str(), cargv.data());
    _exit(127);
  }

  close_fd(&in_pipe[0]);
  close_fd(&out_pipe[1]);
  close_fd(&err_pipe[1]);

  void (*old_pipe)(int) = signal(SIGPIPE, SIG_IGN);
  const uint8_t *p = (const uint8_t *) stdin_data;
  size_t left = stdin_len;
  while (left > 0) {
    ssize_t n = write(in_pipe[1], p, left);
    if (n < 0) {
      if (errno == EINTR) {
        continue;
      }
      break;
    }
    p += (size_t) n;
    left -= (size_t) n;
  }
  close_fd(&in_pipe[1]);
  signal(SIGPIPE, old_pipe);

  read_all_fd(out_pipe[0], &r.out);
  read_all_fd(err_pipe[0], &r.err);
  close_fd(&out_pipe[0]);
  close_fd(&err_pipe[0]);

  int st = 0;
  if (waitpid(pid, &st, 0) < 0) {
    r.status = -1;
    return r;
  }
  if (WIFEXITED(st)) {
    r.status = WEXITSTATUS(st);
  } else if (WIFSIGNALED(st)) {
    r.status = 128 + WTERMSIG(st);
  } else {
    r.status = -1;
  }
  return r;
}

static std::vector<std::string> capnp_cmd(const char *verb,
                                          const std::string &schema,
                                          const char *typ) {
  std::vector<std::string> argv;
  argv.push_back(capnp_bin());
  argv.push_back(verb);
  argv.push_back("-I");
  argv.push_back(include_dir());
  argv.push_back(schema);
  argv.push_back(typ);
  return argv;
}

static std::string read_file_text(const std::string &path) {
  FILE *f = fopen(path.c_str(), "rb");
  if (!f) {
    return "";
  }
  std::string out;
  char buf[512];
  size_t n;
  while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
    out.append(buf, n);
  }
  fclose(f);
  return out;
}

static std::string hex_span(const void *p, size_t n, size_t maxn) {
  const uint8_t *b = (const uint8_t *) p;
  if (n > maxn) {
    n = maxn;
  }
  std::string s;
  char tmp[8];
  for (size_t i = 0; i < n; i++) {
    snprintf(tmp, sizeof(tmp), "%02x%s", b[i], ((i + 1) % 8) == 0 ? " " : "");
    s += tmp;
  }
  return s;
}

static void expect_bytes_eq(const void *got, size_t got_n, const void *want,
                            size_t want_n, const char *label) {
  const uint8_t *g = (const uint8_t *) got;
  const uint8_t *w = (const uint8_t *) want;
  if (got_n == want_n && memcmp(g, w, got_n) == 0) {
    return;
  }
  size_t n = got_n < want_n ? got_n : want_n;
  size_t first = 0;
  while (first < n && g[first] == w[first]) {
    first++;
  }
  ADD_FAILURE() << label << ": size got " << got_n << " want " << want_n
                << " first differ at " << first
                << "\ngot  " << hex_span(g, got_n, 160)
                << "\nwant " << hex_span(w, want_n, 160);
}

static CmdResult capnp_encode_text(const std::string &schema, const char *typ,
                                   const char *text) {
  return run_argv(capnp_cmd("encode", schema, typ), text, strlen(text));
}

static CmdResult capnp_convert(const char *spec, const void *data, size_t len) {
  std::vector<std::string> argv;
  argv.push_back(capnp_bin());
  argv.push_back("convert");
  argv.push_back(spec);
  return run_argv(argv, data, len);
}


#endif /* TESTS_CAPNP_CLI_H */
