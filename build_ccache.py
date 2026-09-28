Import("env")

# Intercept CC (C compiler) and CXX (C++ compiler) calls with ccache
env.Replace(
    CC="ccache " + env.get("CC"),
    CXX="ccache " + env.get("CXX")
)