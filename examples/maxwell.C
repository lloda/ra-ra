
// After Chaitin1986, p. 14. Attempt at straight translation from APL.
// Maxwell -- 4-vector potential vacuum field equations
// (c) Daniel Llorens - 2016

#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include "ra/test.H"
#include "ra/ra-io.H"
#include "ra/ra-operators.H"
#include "ra/format.H"

using real = double;
template <class T, int rank> using array = ra::Owned<T, rank>;
using ra::iota;
auto H = ra::all;
template <int n> constexpr ra::dots_t<n> HH = ra::dots<n>;

auto now() { return std::chrono::high_resolution_clock::now(); }
using time_unit = std::chrono::nanoseconds;
std::string tunit = "ns";

using std::cout; using std::endl;

int main()
{
    TestRecorder tr(cout);

    real delta = 1;
    int o=20, n=20, m=2, l=2;
    array<real, 5> A({o, n, m, l, 4}, 0.);
    array<real, 6> DA({o, n, m, l, 4, 4}, 0.);
    array<real, 6> F({o, n, m, l, 4, 4}, 0.);
    array<real, 4> divA({o, n, m, l}, 0.);
    array<real, 4> X({n, m, l, 4}, 0.), Y({n, m, l, 4}, 0.);

    A(0, H, H, H, 2) = -cos(iota(n)*(2*PI/n))/(2*PI/n);
    A(1, H, H, H, 2) = -cos((iota(n)-delta)*(2*PI/n))/(2*PI/n);

    auto t0 = now();
// @TODO this is painful without a roll operator, but a roll operator that creates a temp is unacceptable.
    for (int t=1; t+1<o; ++t) {
// X←(1⌽[0]A[T;;;;])+(1⌽[1]A[T;;;;])+(1⌽[2]A[T;;;;])
        X(iota(n-1)) = A(t, iota(n-1, 1));
        X(n-1) = A(t, 0);
        X(H, iota(m-1)) += A(t, H, iota(m-1, 1));
        X(H, m-1) += A(t, H, 0);
        X(H, H, iota(l-1)) += A(t, H, H, iota(l-1, 1));
        X(H, H, l-1) += A(t, H, H, 0);
// Y←(¯1⌽[0]A[T;;;;])+(¯1⌽[1]A[T;;;;])+(¯1⌽[2]A[T;;;;])
        Y(iota(n-1, 1)) = A(t, iota(n-1));
        Y(0) = A(t, n-1);
        Y(H, iota(m-1, 1)) += A(t, H, iota(m-1));
        Y(H, 0) += A(t, H, m-1);
        Y(H, H, iota(l-1, 1)) += A(t, H, H, iota(l-1));
        Y(H, H, 0) += A(t, H, H, l-1);

        A(t+1) = X + Y - A(t-1) - 4*A(t);
    }
    time_unit time_A = now()-t0;

// @TODO should try to traverse the array once, e.g. explode() = pack(...). The need to wrap around boundaries complicates this greatly.
    auto diff = [&DA, &A, &delta](auto k_)
        {
            constexpr int k = decltype(k_)::value;
            const int o = DA.size(k);
            if (o>=2) {
                DA(HH<k>, iota(o-2, 1), HH<4-k>, k) = (A(HH<k>, iota(o-2, 2)) - A(HH<k>, iota(o-2, 0)))/(2*delta);
                DA(HH<k>, 0, HH<4-k>, k) = (A(HH<k>, 1) - A(HH<k>, o-1))/(2*delta);
                DA(HH<k>, o-1, HH<4-k>, k) = (A(HH<k>, 0) - A(HH<k>, o-2))/(2*delta);
            }
        };

    t0 = now();
    diff(mp::int_t<0>());
    diff(mp::int_t<1>());
    diff(mp::int_t<2>());
    diff(mp::int_t<3>());
    time_unit time_DA = now()-t0;

    F = ra::transpose<0, 1, 2, 3, 5, 4>(DA) - DA;

// abuse shape matching to reduce last axis.
    divA = 0;
    divA += ra::transpose<0, 1, 2, 3, 4, 4>(DA);
    tr.info("Lorentz test max div A (1)").test_eq(0., amax(divA));
// an alternative without a temporary.
    tr.info("Lorentz test max div A (2)")
        .test_eq(0., amax(map([](auto && a) { return sum(a); },
                              ra::transpose<0, 1, 2, 3, 4, 4>(DA).iter<1>())));

    auto show = [&tr, &delta, &o](char const * name, int t, auto && F)
        {
            tr.quiet().test(amin(F)>=-1);
            tr.quiet().test(amax(F)<=+1);
            cout << name << " t= " << (t*delta) << ":\n";
            for_each([](auto && F) { cout << std::string(int(round((max(min(F, 1.), -1.)+1)*20)), ' ') << "*\n"; }, F);
        };

    for (int t=0; t<o; ++t) {
        show("Ey", t, F(t, H, 0, 0, 2, 0));
        show("Bz", t, F(t, H, 0, 0, 2, 1));
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        cout << endl;
    }
    cout << int(time_A.count()/double(divA.size())) << " " << tunit << " time_A" << endl;
    cout << int(time_DA.count()/double(divA.size())) << " " << tunit << " time_DA" << endl;

    return tr.summary();
}
