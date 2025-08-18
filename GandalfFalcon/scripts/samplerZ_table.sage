
from sage.rings.real_mpfr import RRtoRR
RNprecision = 256
RN = RealField(RNprecision)

tau = RN(1.22)

secpar = RN(128)
qsgn = RN(2) ^ 64
N = RN(512)
epsilon = RN(1 / sqrt(qsgn * secpar))
deltakl = RN( RN(2) * epsilon ^ 2)
a = RN( RN(2) * secpar)

alpha = RN(1.17)
q = RN(12289)

smoothingparameter = RN( RN(1) / pi * sqrt(ln(RN(4) * N * (RN(1) + RN(1) / epsilon)) / RN(2)))
smoothingparameter_square = RN(smoothingparameter^2)
standarddev = RN(smoothingparameter * alpha * sqrt(q))

kappa = RN(2)

beta = RN(tau * standarddev * sqrt((kappa + RN(1)) * N))

print(f"// smoothing parameter {float(smoothingparameter)}")
print(f"// square of smoothing parameter {float(smoothingparameter_square)}")
print(f"// standard deviation {float(standarddev)}")


def get_HalfGaussian_Mitaka_prob(x):
    return RN(RN(2) * e^(-(x^RN(2)) / (RN(2) * RN(1.7424))) / (RN(1.32) * sqrt(RN(2) * pi) + RN(1)))

def get_Gaussian_Gandalf_prob(x):
    return e^(-(x^RN(2)) / (RN(2) * smoothingparameter_square) ) / (smoothingparameter * sqrt(RN(2) * pi))

def get_HalfGaussian_Gandalf_prob(x):
    return RN(2) * e^(-(x^RN(2)) / (RN(2) * smoothingparameter_square)) / (smoothingparameter * sqrt(RN(2) * pi) + RN(1))

def get_Gaussian_prob(x, sigma):
    return e^(-(x^RN(2)) / (RN(2) * sigma * sigma) ) / ((sigma * sqrt(RN(2) * pi)))

def get_HalfGaussian_prob(x, sigma):
    return RN(2) * e^(-(x^RN(2)) / (RN(2) * sigma * sigma)) / (sigma * sqrt(RN(2) * pi) + RN(1))

def to64ibit(x):
    return int(x * RN(2)^64)

prob = RN(0)
acc = RN(0)

gauss0 = RN(get_Gaussian_prob(RN(0), standarddev))
pos_mass = (RN(1) - gauss0) / RN(2)

table_size = 1308

print(f"#define GANDALF_TABLE_SIZE ({table_size})")

print("uint64_t Gandalf_table[GANDALF_TABLE_SIZE] = {")

#define GANDALF_TABLE_SIZE (1308)

# uint64_t Gandalf_table[GANDALF_TABLE_SIZE] = {

for i in range(1, table_size - 1):
    prob = get_Gaussian_prob(RN(i), standarddev)
    acc = acc + prob
    print( f"{to64ibit(acc / pos_mass)}ULL, " )
print( f"{2^64 - 1}ULL")
print("};")








