import FixedPoint::*;
import FIFO::*;
import GetPut::*;
import StmtFSM::*;
import Vector::*;
import GetPut::*;
import ClientServer::*; 

// import InverseSqRoot::*;
// import InverseSqRootParams::*;

// import ProtocolParameters::*;

// Local includes
`include "asim/provides/airblue_parameters.bsh"
`include "asim/provides/airblue_inverse_sq_root.bsh"


module mkHWOnlyApplication();
  Reg#(Bit#(8)) counter <- mkReg(0);

  Tuple3#(FixedPoint#(ISRIPrec,ISRFPrec),
          FixedPoint#(ISRIPrec,ISRFPrec),
          FixedPoint#(ISRIPrec,ISRFPrec)) testVecs[255] = {tuple3(0.007810,0.000061,16394.491364),
                       tuple3(0.015620,0.000244,4098.622841),
                       tuple3(0.023430,0.000549,1821.610152),
                       tuple3(0.031240,0.000976,1024.655710),
                       tuple3(0.039050,0.001525,655.779655),
                       tuple3(0.046860,0.002196,455.402538),
                       tuple3(0.054670,0.002989,334.581456),
                       tuple3(0.062480,0.003904,256.163928),
                       tuple3(0.070290,0.004941,202.401128),
                       tuple3(0.078100,0.006100,163.944914),
                       tuple3(0.085910,0.007381,135.491664),
                       tuple3(0.093720,0.008783,113.850634),
                       tuple3(0.101530,0.010308,97.008825),
                       tuple3(0.109340,0.011955,83.645364),
                       tuple3(0.117150,0.013724,72.864406),
                       tuple3(0.124960,0.015615,64.040982),
                       tuple3(0.132770,0.017628,56.728344),
                       tuple3(0.140580,0.019763,50.600282),
                       tuple3(0.148390,0.022020,45.414104),
                       tuple3(0.156200,0.024398,40.986228),
                       tuple3(0.164010,0.026899,37.175717),
                       tuple3(0.171820,0.029522,33.872916),
                       tuple3(0.179630,0.032267,30.991477),
                       tuple3(0.187440,0.035134,28.462659),
                       tuple3(0.195250,0.038123,26.231186),
                       tuple3(0.203060,0.041233,24.252206),
                       tuple3(0.210870,0.044466,22.489014),
                       tuple3(0.218680,0.047821,20.911341),
                       tuple3(0.226490,0.051298,19.494044),
                       tuple3(0.234300,0.054896,18.216102),
                       tuple3(0.242110,0.058617,17.059825),
                       tuple3(0.249920,0.062460,16.010245),
                       tuple3(0.257730,0.066425,15.054629),
                       tuple3(0.265540,0.070511,14.182086),
                       tuple3(0.273350,0.074720,13.383258),
                       tuple3(0.281160,0.079051,12.650070),
                       tuple3(0.288970,0.083504,11.975523),
                       tuple3(0.296780,0.088078,11.353526),
                       tuple3(0.304590,0.092775,10.778758),
                       tuple3(0.312400,0.097594,10.246557),
                       tuple3(0.320210,0.102534,9.752821),
                       tuple3(0.328020,0.107597,9.293929),
                       tuple3(0.335830,0.112782,8.866680),
                       tuple3(0.343640,0.118088,8.468229),
                       tuple3(0.351450,0.123517,8.096045),
                       tuple3(0.359260,0.129068,7.747869),
                       tuple3(0.367070,0.134740,7.421680),
                       tuple3(0.374880,0.140535,7.115665),
                       tuple3(0.382690,0.146452,6.828193),
                       tuple3(0.390500,0.152490,6.557797),
                       tuple3(0.398310,0.158651,6.303149),
                       tuple3(0.406120,0.164933,6.063052),
                       tuple3(0.413930,0.171338,5.836416),
                       tuple3(0.421740,0.177865,5.622254),
                       tuple3(0.429550,0.184513,5.419667),
                       tuple3(0.437360,0.191284,5.227835),
                       tuple3(0.445170,0.198176,5.046012),
                       tuple3(0.452980,0.205191,4.873511),
                       tuple3(0.460790,0.212327,4.709707),
                       tuple3(0.468600,0.219586,4.554025),
                       tuple3(0.476410,0.226966,4.405937),
                       tuple3(0.484220,0.234469,4.264956),
                       tuple3(0.492030,0.242094,4.130635),
                       tuple3(0.499840,0.249840,4.002561),
                       tuple3(0.507650,0.257709,3.880353),
                       tuple3(0.515460,0.265699,3.763657),
                       tuple3(0.523270,0.273811,3.652148),
                       tuple3(0.531080,0.282046,3.545521),
                       tuple3(0.538890,0.290402,3.443497),
                       tuple3(0.546700,0.298881,3.345815),
                       tuple3(0.554510,0.307481,3.252230),
                       tuple3(0.562320,0.316204,3.162518),
                       tuple3(0.570130,0.325048,3.076467),
                       tuple3(0.577940,0.334015,2.993881),
                       tuple3(0.585750,0.343103,2.914576),
                       tuple3(0.593560,0.352313,2.838381),
                       tuple3(0.601370,0.361646,2.765136),
                       tuple3(0.609180,0.371100,2.694690),
                       tuple3(0.616990,0.380677,2.626901),
                       tuple3(0.624800,0.390375,2.561639),
                       tuple3(0.632610,0.400195,2.498779),
                       tuple3(0.640420,0.410138,2.438205),
                       tuple3(0.648230,0.420202,2.379807),
                       tuple3(0.656040,0.430388,2.323482),
                       tuple3(0.663850,0.440697,2.269134),
                       tuple3(0.671660,0.451127,2.216670),
                       tuple3(0.679470,0.461679,2.166005),
                       tuple3(0.687280,0.472354,2.117057),
                       tuple3(0.695090,0.483150,2.069750),
                       tuple3(0.702900,0.494068,2.024011),
                       tuple3(0.710710,0.505109,1.979772),
                       tuple3(0.718520,0.516271,1.936967),
                       tuple3(0.726330,0.527555,1.895536),
                       tuple3(0.734140,0.538962,1.855420),
                       tuple3(0.741950,0.550490,1.816564),
                       tuple3(0.749760,0.562140,1.778916),
                       tuple3(0.757570,0.573912,1.742427),
                       tuple3(0.765380,0.585807,1.707048),
                       tuple3(0.773190,0.597823,1.672737),
                       tuple3(0.781000,0.609961,1.639449),
                       tuple3(0.788810,0.622221,1.607146),
                       tuple3(0.796620,0.634603,1.575787),
                       tuple3(0.804430,0.647108,1.545338),
                       tuple3(0.812240,0.659734,1.515763),
                       tuple3(0.820050,0.672482,1.487029),
                       tuple3(0.827860,0.685352,1.459104),
                       tuple3(0.835670,0.698344,1.431958),
                       tuple3(0.843480,0.711458,1.405563),
                       tuple3(0.851290,0.724695,1.379892),
                       tuple3(0.859100,0.738053,1.354917),
                       tuple3(0.866910,0.751533,1.330614),
                       tuple3(0.874720,0.765135,1.306959),
                       tuple3(0.882530,0.778859,1.283929),
                       tuple3(0.890340,0.792705,1.261503),
                       tuple3(0.898150,0.806673,1.239659),
                       tuple3(0.905960,0.820763,1.218378),
                       tuple3(0.913770,0.834976,1.197640),
                       tuple3(0.921580,0.849310,1.177427),
                       tuple3(0.929390,0.863766,1.157721),
                       tuple3(0.937200,0.878344,1.138506),
                       tuple3(0.945010,0.893044,1.119766),
                       tuple3(0.952820,0.907866,1.101484),
                       tuple3(0.960630,0.922810,1.083647),
                       tuple3(0.968440,0.937876,1.066239),
                       tuple3(0.976250,0.953064,1.049247),
                       tuple3(0.984060,0.968374,1.032659),
                       tuple3(0.991870,0.983806,1.016460),
                       tuple3(0.999680,0.999360,1.000640),
                       tuple3(1.007490,1.015036,0.985187),
                       tuple3(1.015300,1.030834,0.970088),
                       tuple3(1.023110,1.046754,0.955334),
                       tuple3(1.030920,1.062796,0.940914),
                       tuple3(1.038730,1.078960,0.926818),
                       tuple3(1.046540,1.095246,0.913037),
                       tuple3(1.054350,1.111654,0.899561),
                       tuple3(1.062160,1.128184,0.886380),
                       tuple3(1.069970,1.144836,0.873488),
                       tuple3(1.077780,1.161610,0.860874),
                       tuple3(1.085590,1.178506,0.848532),
                       tuple3(1.093400,1.195524,0.836454),
                       tuple3(1.101210,1.212663,0.824631),
                       tuple3(1.109020,1.229925,0.813057),
                       tuple3(1.116830,1.247309,0.801726),
                       tuple3(1.124640,1.264815,0.790629),
                       tuple3(1.132450,1.282443,0.779762),
                       tuple3(1.140260,1.300193,0.769117),
                       tuple3(1.148070,1.318065,0.758688),
                       tuple3(1.155880,1.336059,0.748470),
                       tuple3(1.163690,1.354174,0.738457),
                       tuple3(1.171500,1.372412,0.728644),
                       tuple3(1.179310,1.390772,0.719025),
                       tuple3(1.187120,1.409254,0.709595),
                       tuple3(1.194930,1.427858,0.700350),
                       tuple3(1.202740,1.446583,0.691284),
                       tuple3(1.210550,1.465431,0.682393),
                       tuple3(1.218360,1.484401,0.673672),
                       tuple3(1.226170,1.503493,0.665118),
                       tuple3(1.233980,1.522707,0.656725),
                       tuple3(1.241790,1.542042,0.648491),
                       tuple3(1.249600,1.561500,0.640410),
                       tuple3(1.257410,1.581080,0.632479),
                       tuple3(1.265220,1.600782,0.624695),
                       tuple3(1.273030,1.620605,0.617053),
                       tuple3(1.280840,1.640551,0.609551),
                       tuple3(1.288650,1.660619,0.602185),
                       tuple3(1.296460,1.680808,0.594952),
                       tuple3(1.304270,1.701120,0.587848),
                       tuple3(1.312080,1.721554,0.580871),
                       tuple3(1.319890,1.742110,0.574017),
                       tuple3(1.327700,1.762787,0.567283),
                       tuple3(1.335510,1.783587,0.560668),
                       tuple3(1.343320,1.804509,0.554168),
                       tuple3(1.351130,1.825552,0.547779),
                       tuple3(1.358940,1.846718,0.541501),
                       tuple3(1.366750,1.868005,0.535330),
                       tuple3(1.374560,1.889415,0.529264),
                       tuple3(1.382370,1.910947,0.523301),
                       tuple3(1.390180,1.932600,0.517438),
                       tuple3(1.397990,1.954376,0.511672),
                       tuple3(1.405800,1.976274,0.506003),
                       tuple3(1.413610,1.998293,0.500427),
                       tuple3(1.421420,2.020435,0.494943),
                       tuple3(1.429230,2.042698,0.489549),
                       tuple3(1.437040,2.065084,0.484242),
                       tuple3(1.444850,2.087591,0.479021),
                       tuple3(1.452660,2.110221,0.473884),
                       tuple3(1.460470,2.132973,0.468829),
                       tuple3(1.468280,2.155846,0.463855),
                       tuple3(1.476090,2.178842,0.458959),
                       tuple3(1.483900,2.201959,0.454141),
                       tuple3(1.491710,2.225199,0.449398),
                       tuple3(1.499520,2.248560,0.444729),
                       tuple3(1.507330,2.272044,0.440132),
                       tuple3(1.515140,2.295649,0.435607),
                       tuple3(1.522950,2.319377,0.431150),
                       tuple3(1.530760,2.343226,0.426762),
                       tuple3(1.538570,2.367198,0.422440),
                       tuple3(1.546380,2.391291,0.418184),
                       tuple3(1.554190,2.415506,0.413992),
                       tuple3(1.562000,2.439844,0.409862),
                       tuple3(1.569810,2.464303,0.405794),
                       tuple3(1.577620,2.488885,0.401786),
                       tuple3(1.585430,2.513588,0.397838),
                       tuple3(1.593240,2.538414,0.393947),
                       tuple3(1.601050,2.563361,0.390113),
                       tuple3(1.608860,2.588430,0.386335),
                       tuple3(1.616670,2.613622,0.382611),
                       tuple3(1.624480,2.638935,0.378941),
                       tuple3(1.632290,2.664371,0.375323),
                       tuple3(1.640100,2.689928,0.371757),
                       tuple3(1.647910,2.715607,0.368242),
                       tuple3(1.655720,2.741409,0.364776),
                       tuple3(1.663530,2.767332,0.361359),
                       tuple3(1.671340,2.793377,0.357990),
                       tuple3(1.679150,2.819545,0.354667),
                       tuple3(1.686960,2.845834,0.351391),
                       tuple3(1.694770,2.872245,0.348160),
                       tuple3(1.702580,2.898779,0.344973),
                       tuple3(1.710390,2.925434,0.341830),
                       tuple3(1.718200,2.952211,0.338729),
                       tuple3(1.726010,2.979110,0.335671),
                       tuple3(1.733820,3.006132,0.332653),
                       tuple3(1.741630,3.033275,0.329677),
                       tuple3(1.749440,3.060540,0.326740),
                       tuple3(1.757250,3.087927,0.323842),
                       tuple3(1.765060,3.115437,0.320982),
                       tuple3(1.772870,3.143068,0.318160),
                       tuple3(1.780680,3.170821,0.315376),
                       tuple3(1.788490,3.198696,0.312627),
                       tuple3(1.796300,3.226694,0.309915),
                       tuple3(1.804110,3.254813,0.307237),
                       tuple3(1.811920,3.283054,0.304594),
                       tuple3(1.819730,3.311417,0.301986),
                       tuple3(1.827540,3.339902,0.299410),
                       tuple3(1.835350,3.368510,0.296867),
                       tuple3(1.843160,3.397239,0.294357),
                       tuple3(1.850970,3.426090,0.291878),
                       tuple3(1.858780,3.455063,0.289430),
                       tuple3(1.866590,3.484158,0.287013),
                       tuple3(1.874400,3.513375,0.284627),
                       tuple3(1.882210,3.542714,0.282269),
                       tuple3(1.890020,3.572175,0.279941),
                       tuple3(1.897830,3.601759,0.277642),
                       tuple3(1.905640,3.631464,0.275371),
                       tuple3(1.913450,3.661291,0.273128),
                       tuple3(1.921260,3.691240,0.270912),
                       tuple3(1.929070,3.721311,0.268723),
                       tuple3(1.936880,3.751504,0.266560),
                       tuple3(1.944690,3.781819,0.264423),
                       tuple3(1.952500,3.812256,0.262312),
                       tuple3(1.960310,3.842815,0.260226),
                       tuple3(1.968120,3.873496,0.258165),
                       tuple3(1.975930,3.904299,0.256128),
                       tuple3(1.983740,3.935224,0.254115),
                       tuple3(1.991550,3.966271,0.252126)};



  function Vector#(size,Real) genRealVec;
    Vector#(size,Real) vec = newVector;
    Real  r = 1.0;
    for(Integer i = 0; i < valueof(size); i = i+1)
       begin
         vec[i] = r;
         r = r + 1;
       end
    return vec;
  endfunction
  

  
 InverseSqRoot#(ISRIPrec,ISRFPrec) invSqRoot <- mkSimpleInverseSqRoot;  

 match {.root, .square, .invroot} = testVecs[counter]; 
 Stmt s = seq 
            while(counter + 1 != 0) 
              seq
                invSqRoot.request.put(fxptTruncate(square));
                action
                  let result <- invSqRoot.response.get;
                  $write("Root: ");
                  FixedPoint#(ISRIPrec,ISRFPrec) fp = root; 
                  fxptWrite(5,fp);
                  $write(" Square: ");
                  fp = square;
                  fxptWrite(5,fp);
                  $write(" Mult: ");
                  fp = root * result;
                  fxptWrite(5,fp);
                  $write(" Result: ");
                  fxptWrite(5,result);
                  $display("");
                endaction
                counter <= counter + 1;
              endseq
            $finish;
          endseq;

  FSM fsm <- mkFSM(s);

  rule testNR(fsm.done);
    fsm.start;
  endrule
          
endmodule