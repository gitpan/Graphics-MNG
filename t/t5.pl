#!perl 
# Before `make install' is performed this script should be runnable with
# `make test'. After `make install' it should work as `perl test.pl'

#########################

# change 'tests => 1' to 'tests => last_test_to_print';

use strict;
use Test;
BEGIN { plan tests => 26 };
use Graphics::MNG;
ok(1); # If we made it this far, we're ok.

#########################

# Insert your test code below, the Test module is use()ed here so read
# its man page ( perldoc Test ) for help writing this test script.

use Graphics::MNG qw( MNG_FUNCTIONINVALID );
ok(1);   # loaded an export-ok constant

use FileHandle;
use Cwd;
use Data::Dumper;
  use constant FILENAME     => 'counter.mng';
# use constant FILENAME     => 'linux.mng';
  use constant OUT_FILENAME => 'tempfile.mng';


# open(STDERR,">log.txt");
main();
exit(0);


sub get_filename($)
{
   my ($fn) = @_;
   my ($match) = grep { -r $_ } ( $fn, "t/$fn" );
   return $match || ( -d 't' ? "t/$fn" : $fn );
}

sub main
{
   my $input  = readfile(FILENAME);
   my @chunk1 = @{ $input->get_userdata()->{'chunks'} };

   my $output = writefile(OUT_FILENAME, FILENAME, \@chunk1 );

   my $reread = readfile(OUT_FILENAME);
   my @chunk2 = @{ $reread->get_userdata()->{'chunks'} };

   my $len1 = int @chunk1;
   my $len2 = int @chunk2;
   my $minlen = $len1 < $len2 ? $len1 : $len2;

   foreach my $chunknum (0..$minlen-1)
   {
      my ($chunk1,$chunk2) = (@chunk1->[$chunknum],@chunk2->[$chunknum]);
      my ($name1, $name2 ) = ($chunk1->{'pChunkname'}||'?', $chunk2->{'pChunkname'}||'?');

      if ( $name1 ne $name2 )
      {
         warn("Iteration $chunknum: Chunk #1 name is $name1, Chunk #2 name is $name2\n");
      }
   }

   # clean up
   unlink( get_filename(OUT_FILENAME) );
}

sub readfile
{
   my ($fn) = @_;
   my $rv;
   my $obj;
   my $userdata;
   my @chunks;

   $obj = Graphics::MNG::new();
   $obj->set_userdata( { 'filename' => $fn,
                         'fh'       => undef,
                         'fperms'   => 'r',
                         'width'    => 0,
                         'height'   => 0,
                         'chunks'   => [],
                       } );

   $rv = defined $obj ? 1 : 0;
   ok($rv,1,"creating the input object");

   $rv = $obj->setcb_openstream( \&OpenStream    );
   ok($rv,MNG_NOERROR,"registering the openstream callback");

   $rv = $obj->setcb_closestream( \&CloseStream   );
   ok($rv,MNG_NOERROR,"registering the closestream callback");

   $rv = $obj->setcb_processheader( \&ProcessHeader );
   ok($rv,MNG_NOERROR,"registering the processheader callback");

   $rv = $obj->setcb_readdata( \&FileReadData  );
   ok($rv,MNG_NOERROR,"registering the filereaddata callback");

   # read the file into memory
   $rv = $obj->read();
   ok($rv,MNG_NOERROR,"reading the file");

   # run through the chunk list
   $rv = $obj->iterate_chunks(0, \&IterateChunks );
   ok($rv,MNG_NOERROR,"iterating the chunks");


   @chunks = @{ $obj->get_userdata()->{'chunks'} };
   ok(@chunks?1:0,1,"checking chunk count");

   return $obj;
}

sub writefile
{
   my ($fn, $orig, $chunkRef) = @_;
   my $rv;
   my $obj;
   my $userdata;
   my @chunks = @{ $chunkRef };

   # delete the output filename if it exists
   unlink($fn) if (-e $fn);


   $obj = Graphics::MNG::new();
   $rv = defined $obj ? 1 : 0;
   $obj->set_userdata( { 'filename' => $fn,
                         'original' => $orig,
                         'fh'       => undef,
                         'fperms'   => 'w+',
                         'width'    => 0,
                         'height'   => 0,
                       } );

   ok($rv,1,"creating the output object");

   $rv = $obj->setcb_openstream( \&OpenStream    );
   ok($rv,MNG_NOERROR,"registering the openstream callback");

   $rv = $obj->setcb_closestream( \&CloseStream   );
   ok($rv,MNG_NOERROR,"registering the closestream callback");

   $rv = $obj->setcb_writedata( \&FileWriteData  );
   ok($rv,MNG_NOERROR,"registering the filewritedata callback");

   # get a handle to my output object's user data
   $userdata = $obj->get_userdata();

   # indicate that we're going to make a new file...
   $rv = $obj->create();
   ok($rv,MNG_NOERROR,"creating the file");

   # now see if we can write out all of those chunks...
   foreach my $chunknum (0..@chunks-1)
   {
      my $chunk = @chunks->[$chunknum];

      warn("Got a hash w/o a type\n" . Dumper($chunk)) if ( !exists $chunk->{'iChunktype'} );

      $rv = $obj->putchunk_info($chunk);
      warn("putchunk_info() failed ($rv)\n") unless defined $rv && $rv==MNG_NOERROR;

      # don't print...
   #  warn Dumper( { 'rv' => $rv, 'chunk' => $chunk } );
   }


   ok($rv,MNG_NOERROR,"writing all chunks");

   # now write the file
   $rv = $obj->write();
   ok($rv,MNG_NOERROR,"writing the file");

   # check to see if the file is there and matches
   $rv = compare_files( $fn, $orig );
   ok($rv,0,"in/out file comparsion");

   return $obj;
}




#---------------------------------------------------------------------------
sub FileReadData
{
   my ( $hHandle, $pBuf, $iSize, $pRead ) = @_;
   my $userdata = $hHandle->get_userdata();

   # don't mix sysread() and syswrite() with anything else
   $$pRead = sysread( $userdata->{'fh'}, $$pBuf, $iSize );
 # $$pRead = read( $userdata->{'fh'}, $$pBuf, $iSize );

 # warn("read $$pRead / $iSize bytes\n");
   return MNG_TRUE;
}

#---------------------------------------------------------------------------
sub FileWriteData
{
   my ( $hHandle, $pBuf, $iBuflen, $pWritten ) = @_;
   my $userdata = $hHandle->get_userdata();

   # don't mix sysread() and syswrite() with anything else
   $$pWritten = syswrite($userdata->{'fh'}, $pBuf, $iBuflen);
 # $$pWritten = $userdata->{'fh'}->print( $pBuf );

 # warn("wrote $$pWritten / $iBuflen bytes\n");
   return MNG_TRUE;
}

#---------------------------------------------------------------------------
sub ProcessHeader
{
   my ( $hHandle, $iWidth, $iHeight ) = @_;
   my $userdata = $hHandle->get_userdata();

 # warn("Processing file header\n");
   $userdata->{'width'}  = $iWidth;
   $userdata->{'height'} = $iHeight;
   return MNG_TRUE;
}

#---------------------------------------------------------------------------
sub OpenStream
{
   my ( $hHandle ) = @_;
   my $userdata = $hHandle->get_userdata();
   my $fn       = get_filename( $userdata->{'filename'} );
   my $fperms   = $userdata->{'fperms'} || 'r';
   my $rv       = MNG_FALSE;

 # warn("Opening file $fn with perms '$fperms' via callback");

   my $fh = new FileHandle( $fn, $fperms );

   $userdata->{'fh'} = $fh;

   if ( defined $fh )
   {
      $fh->autoflush(1);
      binmode($fh);
      $rv = MNG_TRUE;
   }

   return $rv;
}

#---------------------------------------------------------------------------
sub CloseStream
{
   my ( $hHandle ) = @_;
   my $userdata = $hHandle->get_userdata();

 # warn("Closing file...\n");

   $userdata->{'fh'}->close();
   $userdata->{'fh'} = undef;

   return MNG_TRUE;
}

#---------------------------------------------------------------------------
sub IterateChunks
{
   my ( $hHandle, $hChunk, $iChunktype, $iChunkseq ) = @_;
   my $userdata = $hHandle->get_userdata();

   my ($name,$type) = $hHandle->getchunk_name($iChunktype);

   my ($rv, $info) = $hHandle->getchunk_info($hChunk,$iChunktype);

   # add the sequence information
   $info ||= {};
   %$info->{'iChunkseq'} = $iChunkseq;

   # store the chunk
   push( @{$userdata->{'chunks'}}, $info ) if defined $rv && $rv==MNG_NOERROR();

   # print out the length in the description
   my ($len) = map { $$info{$_} } ( grep { /len/i } keys %$info );
   $len ||= '';
   my $desc = "Chunk $iChunkseq: 0x$type/$name/$len/$rv";

 # warn("Chunk $iChunkseq:\n" . Dumper($info)) if $info->{'pChunkname'} =~ /END$/ ;
 # warn("$desc\n")                             if $iChunkseq <= 7;
 # warn("Chunk $iChunkseq:\n" . Dumper($info)) if $iChunkseq <= 7;

   return MNG_TRUE;
}


#---------------------------------------------------------------------------
sub compare_files
{
   use FileHandle;
   my ( $f1, $f2 ) = @_;

   return "missing $f1" unless ( -e get_filename($f1) );
   return "missing $f2" unless ( -e get_filename($f2) );

   local ( $/ ) = undef;
   my @data;

   foreach my $fn ( map { get_filename($_) } ( $f1, $f2 ) )
   {
      my $fh = new FileHandle($fn);
      if ( $fh )
      {
         binmode $fh;
         my $data = <$fh>;
         push( @data, $data );
      }
      undef $fh;
   }

   warn("Didn't read both files $f1 and $f2\n") unless ( @data >= 2 );
   warn("Length of $f1 != length of $f2\n")
      if ( length($data[0]) != length($data[1]) );

   my $rv = $data[0] cmp $data[1];
   return $rv;
}



