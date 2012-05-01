def set_options(opt):
  opt.tool_options('compiler_cxx')

def configure(conf):
  conf.check_tool('compiler_cxx')
  conf.check_tool('node_addon')
  conf.check_cfg(package='augeas',
                atleast_version='0.10',
                args='--cflags --libs',
                mandatory=True)

  # libxml2 is required since 0.10:
  conf.check_cfg(package='libxml-2.0',
                args='--cflags --libs',
                mandatory=True)
def build(bld):
  obj = bld.new_task_gen('cxx', 'shlib', 'node_addon')
  obj.target = 'libaugeas'
  obj.source = 'libaugeas.cc'
  obj.uselib = ['AUGEAS', 'LIBXML-2.0']

