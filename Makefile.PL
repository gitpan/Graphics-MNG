use ExtUtils::MakeMaker;
# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.

use strict;
my $namespace = 'Graphics::MNG';
my $author    = 'dpmott@sep.com';
my $modname   = 'MNG.pm';

# this references header files in the package.  You may want to change these...
my @libs = ( '-Llib -llibmng.lib' );
my $incs = ' -I. -Iinclude ';
my $defs = ' -DMNG_FULL_CMS -DMNG_USE_SO -DNON_WINDOWS -DMNG_INTERNAL_MEMMNGMT -DMNG_SUPPORT_TRACE ';

# this is where I build it on my machine...
# my @libs = ( '-L../../../libmng/Release -llibmng.lib' );
# my $incs = ' -I. -I..\..\..\Gd\jpeg-6b -I..\..\..\Gd\zlib-1.1.3 -I..\..\..\libmng\libmng -I..\..\..\libmng\lcms\src ';
# my $defs = ' -DMNG_FULL_CMS -DMNG_USE_SO -DNON_WINDOWS -DMNG_INTERNAL_MEMMNGMT -DMNG_SUPPORT_TRACE ';

# I can't seem to link to the official Win32 libmng.dll ...
# with these settings, and using the libmng.lib from the contrib/msvc/win32dll
# directory, I get 8 linker errors concerning unresolved symbols.
# I have been unable to generate a suitable implib for use with the DLL.
# my @libs = ( '-Llib -llibmng.lib' );
# my $incs = ' -I. -I..\..\..\Gd\jpeg-6b -I..\..\..\Gd\zlib-1.1.3 -I..\..\..\libmng\libmng -I..\..\..\libmng\lcms\src ';
# my $defs = ' -DMNG_FULL_CMS -DMNG_USE_DLL ';



WriteMakefile(
    'NAME'           => $namespace,
    'VERSION_FROM'   => $modname,          # finds $VERSION
    'PREREQ_PM'      => {},                # e.g., Module::Name => 1.1
    ($] >= 5.005 ?                         ## Add these new keywords supported since 5.005
      (ABSTRACT_FROM => $modname,          # retrieve abstract from module
       AUTHOR        => $author) : ()),
    'LIBS'           => [@libs],           # e.g., '-lm'
    'DEFINE'         => $defs,             # e.g., '-DHAVE_SOMETHING'
    'INC'            => $incs,             # e.g., '-I. -I/usr/include/other'

	 # Un-comment this if you add C files to link with later:
    # 'OBJECT'		=> '$(O_FILES)', # link all the C files too
);


if ( open(PM,$modname) )
{
   undef $/;
   my $content = <PM>;
   close PM;

   $content =~ s/^.*?__END__//os;
   $content =~ s/^=head[\d]*\s*//gomsi;
   $content =~ s/=cut.*$//os;
   if ( open(README,">README") )
   {
      my $version = MM->parse_version($modname);
      my $header  = "$namespace version $version\n"; 
      print README $header, '=' x (length($header)-1), "\n";
      print README $content;
      close README;
   }
}

