$bin_dir = "$ENV{srcdir}/../bin";
$tmp = `$bin_dir/createmp elas.param`;

sub catch { `rm -f $tmp`; exit(1); }
$SIG{INT} = 'catch';

open(TMPF, ">$tmp") or die "Open file impossible : $!\n";
print TMPF <<
LX = 1.0;		% size in X.
LY = 1.0;	        % size in Y.
LZ = 1.0;		% size in Z.
MU = 1.0;	        % Lam� coefficient.
LAMBDA = 0.0;   	% Lam� coefficient.
EPSILON = 0.01;          % thickness of the plate
PRESSURE = 0.01;         % pressure on the top surface of the plate.
MESH_TYPE = 'GT_QK(2,1)'; % linear rectangles
NX = 10;            	          % space step.
MESH_NOISE = 0; % Set to one if you want to "shake" the mesh
FEM_TYPE_UT = 'FEM_QK(2,1)';
FEM_TYPE_U3 = 'FEM_QK(2,1)';
FEM_TYPE_THETA= 'FEM_QK(2,2)';
DATA_FEM_TYPE = 'FEM_QK(2,1)';
INTEGRATION = 'IM_GAUSS_PARALLELEPIPED(2,4)';
INTEGRATION_CT = 'IM_GAUSS_PARALLELEPIPED(2,4)';
RESIDU = 1E-9;     	% residu for conjugate gradient.
ROOTFILENAME = 'plate';     % Root of data files.
VTK_EXPORT = 2 % export solution to a .vtk file ?
MIXED = 1;
SYMMETRIZED = 1;

;
close(TMPF);

$er = 0;
open F, "./plate $tmp 2>&1 |" or die;
while (<F>) {
  # print $_;
  if ($_ =~ /error has been detected/)
  {
    $er = 1;
    print "============================================\n";
    print $_, <F>;
  }
}
close(F); if ($?) { `rm -f $tmp`; exit(1); }
if ($er == 1) { `rm -f $tmp`; exit(1); }
`rm -f $tmp`;


