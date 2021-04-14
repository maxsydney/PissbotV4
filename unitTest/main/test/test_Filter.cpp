#include "unity.h"
#include "main/Filter.h"

#ifdef __cplusplus
extern "C" {
#endif

void includeFilterTests(void)
{
    // Dummy function to force discovery of unit tests by main test runner
}

static FilterConfig validConfig(void)
{
    FilterConfig cfg {};
    cfg.num = {1.0};
    cfg.den = {};

    return cfg;
}

TEST_CASE("Constructor", "[Filter]")
{
    // Default object not configured
    {
        Filter filter {};
        TEST_ASSERT_FALSE(filter.isConfigured());
    }

    // Valid params
    {
        Filter filter(validConfig());
        TEST_ASSERT_TRUE(filter.isConfigured());
    }

    // 0 numerator coefficients
    {
        std::vector<double> num {};
        std::vector<double> den {1.0};
        FilterConfig config(num, den);
        Filter filter(config);
        TEST_ASSERT_FALSE(filter.isConfigured());
    }

    // Too many numerator coefficients
    {
        std::vector<double> num {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
        std::vector<double> den {1.0};
        FilterConfig config(num, den);
        Filter filter(config);
        TEST_ASSERT_FALSE(filter.isConfigured());
    }

    // Too many denominator coefficients
    {
        std::vector<double> num {1.0};
        std::vector<double> den {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
        FilterConfig config(num, den);
        Filter filter(config);
        TEST_ASSERT_FALSE(filter.isConfigured());
    }

    // NaN in numerator coeffients
    {
        std::vector<double> num {NAN};
        std::vector<double> den {1.0};
        FilterConfig config(num, den);
        Filter filter(config);
        TEST_ASSERT_FALSE(filter.isConfigured());
    }

    // INF in numerator coefficients
    {
        std::vector<double> num {std::numeric_limits<double>::infinity()};
        std::vector<double> den {1.0};
        FilterConfig config(num, den);
        Filter filter(config);
        TEST_ASSERT_FALSE(filter.isConfigured());
    }

    // NaN in denominator coefficients
    {
        std::vector<double> num {1.0};
        std::vector<double> den {NAN};
        FilterConfig config(num, den);
        Filter filter(config);
        TEST_ASSERT_FALSE(filter.isConfigured());
    }

    // INF in denominator coefficients
    {
        std::vector<double> num {1.0};
        std::vector<double> den {std::numeric_limits<double>::infinity()};
        FilterConfig config(num, den);
        Filter filter(config);
        TEST_ASSERT_FALSE(filter.isConfigured());
    }
}

TEST_CASE("filterValid", "[Filter]")
{
    // Filter returns input with num = {1.0}
    {
        Filter filter(validConfig());
        TEST_ASSERT_TRUE(filter.isConfigured());
        const double tol = 1e-9;

        std::vector<double> in = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0};
        for (double& val : in) {
            double out = 0.0;
            TEST_ASSERT_EQUAL(PBRet::SUCCESS, filter.filter(val, out));
            TEST_ASSERT_DOUBLE_WITHIN(tol, val, out);
        }
    }

    // LPF step response
    {
        std::vector<double> num {0.06745228281719011, 0.13490456563438022, 0.06745228281719011};
        std::vector<double> den {-1.1429298210046335, 0.41273895227339397};
        FilterConfig config(num, den);
        Filter filter(config);
        const double tol = 1e-9;

        // Reference data
        std::vector<double> input = {0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
        std::vector<double> filtered = {0.0, 0.0, 0.0, 0.0, 0.06745228281719011, 0.27945007397817534, 0.5613607697619523, 0.7960651646253372, 0.9479602914230869, 1.0246941154756408, 1.0497024557750898, 1.04661419553378, 1.032762614635673, 1.018205875055514, 1.0072856302799666, 1.000812690316155, 0.9979217845891704, 0.9972893166828789, 0.9977596396528268, 0.9985582299416729};
    
        for (size_t i = 0; i < input.size(); i++) {
            double out = 0.0;
            TEST_ASSERT_EQUAL(PBRet::SUCCESS, filter.filter(input[i], out));
            TEST_ASSERT_DOUBLE_WITHIN(tol, filtered[i], out);
        }
    }

    // LPF impulse response
    {
        std::vector<double> num {0.06745228281719011, 0.13490456563438022, 0.06745228281719011};
        std::vector<double> den {-1.1429298210046335, 0.41273895227339397};
        FilterConfig config(num, den);
        Filter filter(config);
        const double tol = 1e-9;

        // Reference data
        std::vector<double> input = {100.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
        std::vector<double> filtered = {6.745228281719011, 21.199779116098522, 28.19106957837769, 23.4704394863385, 15.189512679774984, 7.673382405255385, 2.5008340299448815, -0.30882602413100413, -1.3851580898107212, -1.455673958015899, -1.092024477554725, -0.6472939963811535, -0.28909057269844796, -0.06324679062914873, 0.047032296994782355, 0.07985902888461291, 0.07186120460405693, 0.04917138180388629, 0.0265395203062838, 0.010037864585638726};
            
        for (size_t i = 0; i < input.size(); i++) {
            double out = 0.0;
            TEST_ASSERT_EQUAL(PBRet::SUCCESS, filter.filter(input[i], out));
            TEST_ASSERT_DOUBLE_WITHIN(tol, filtered[i], out);
        }
    }

    // HPF step response
    {
        std::vector<double> num {0.39131201, -0.78262402, 0.39131201};
        std::vector<double> den {-0.36950494, 0.19574310};
        FilterConfig config(num, den);
        Filter filter(config);
        const double tol = 1e-9;

        // Reference data
        std::vector<double> input = {0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
        std::vector<double> filtered = {0.0, 0.0, 0.0, 0.0, 0.39131201, -0.2467202892236706, -0.16776099157100605, -0.013694720879247217, 0.027777789532379603, 0.01294467757303313, -0.0006541883242725819, -0.0027755571341550223, -0.0008975292217456031, 0.0002116546764372642, 0.00025389260072274247, 5.238462770117387e-05, -3.034134601688726e-05, -2.1465206658062813e-05, -1.9923907707569357e-06, 3.46546786118476e-06};

        for (size_t i = 0; i < input.size(); i++) {
            double out = 0.0;
            TEST_ASSERT_EQUAL(PBRet::SUCCESS, filter.filter(input[i], out));
            TEST_ASSERT_DOUBLE_WITHIN(tol, filtered[i], out);
        }
    }

    // HPF impulse response
    {
        std::vector<double> num {0.39131201, -0.78262402, 0.39131201};
        std::vector<double> den {-0.36950494, 0.19574310};
        FilterConfig config(num, den);
        Filter filter(config);
        const double tol = 1e-9;

        // Reference data
        std::vector<double> input = {100.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
        std::vector<double> filtered = {39.131201, -63.80322992236705, 7.895929765266455, 15.406627069175883, 4.147251041162683, -1.483311195934647, -1.359886589730571, -0.21213688098824407, 0.1878027912409419, 0.11091838981828672, 0.004223792428547833, -0.020150797302156857, -0.008272597371806113, 0.0008876139358824447, 0.0019472815887305879, 0.0005457858631941696, -0.00017949636211864048, -0.0001731586093126692, -2.8847787184736012e-05, 2.3235203105722084e-05};

        for (size_t i = 0; i < input.size(); i++) {
            double out = 0.0;
            TEST_ASSERT_EQUAL(PBRet::SUCCESS, filter.filter(input[i], out));
            TEST_ASSERT_DOUBLE_WITHIN(tol, filtered[i], out);
        }
    }
}

#ifdef __cplusplus
}
#endif