project('POC_RtAudio', 'cpp',
    default_options: ['cpp_std=c++17'])
    
compiler = meson.get_compiler('cpp')

inc = include_directories(
    './include',
    './portaudio/include'
)

defines = [
    '-g',
    '-Wall',
    '-Wno-deprecated-declarations',
    '-D__MACOSX_CORE__'
]

#deps = [
#    dependency('CoreAudio'),
#    dependency('CoreFoundation'),
#]	

srcs =  [
    'src/main.cpp',
    'src/StreamHandler.cpp',
    'src/Compressor.cpp'
]

executable('POC_RtAudio', 
    srcs,
    include_directories: inc,
    #dependencies: deps,
    link_args : ['-l' + 'portaudio'],
    cpp_args: defines
)
