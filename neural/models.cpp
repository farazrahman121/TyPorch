#include "./../linalg/linalg.h"
#include "neural.h"
#include "functions.cpp"
#include <cmath>
#include <functional>

using std::string;
using std::vector;
using std::cout;
using std::function;

#define OUTPUT_NOTES 0
#define OUTPUT_WARNINGS 1


// **LOSS FUNCTION**

LossFunction::LossFunction(string i_name) {
    
    if(i_name == "MSE" || i_name == "Mean Squared Error") {
        name = "MSE - Mean Squared Error";
        loss = [](const Tensor& pred,  const Tensor& target) {
            return computeElementWise(squaredError, pred, target).getMean();
        };
        lossPrime = [](const Tensor& pred,  const Tensor& target) {
            return computeElementWise(squaredErrorPrime, pred, target) / target.getSize();
        };

    }

    else if(i_name == "SSE" || i_name == "Sum Squared Error") {
        name = "SSE - Sum of Squared Error";
        loss = [](const Tensor& pred,  const Tensor& target) {
            return computeElementWise(squaredError, pred, target).getSum();
        };
        lossPrime = [](const Tensor& pred,  const Tensor& target) {
            return computeElementWise(squaredErrorPrime, pred, target);
        };

    }

    else if(i_name == "CE" || i_name == "Cross Entropy") {
        name = "CE - Cross Entropy";

        loss = [](const Tensor& pred,  const Tensor& target) { 
            return computeElementWise(crossEntropy, pred, target).getSum();
        };
        lossPrime = [](const Tensor& pred, const Tensor& target) {
            return computeElementWise(crossEntropyPrime, pred, target);
        };

    }

    else {
        cout << "ERROR: \"" << i_name << "\" does not match any preset loss function\n";
        exit(EXIT_FAILURE);
    }

}
double LossFunction::getLoss(const Tensor& pred, const Tensor& target) const {
    if(pred.getShape() != target.getShape()) {
        cout << "ERROR: prediction Tensor and target Tensor shape mismatch\n";
        exit(EXIT_FAILURE);
    }
    return loss(pred, target);
}
Tensor LossFunction::getGradient(const Tensor& pred, const Tensor& target) const {
    if(pred.getShape() != target.getShape()) {
        cout << "ERROR: prediction Tensor and target Tensor shape mismatch\n";
        exit(EXIT_FAILURE);
    }
    return lossPrime(pred, target);
}


// **OPTIMIZER**

Optimizer::Optimizer(string i_name, vector<double> i_parameters) {
    parameters = i_parameters;

    if(i_name == "SGD") {
        name = "SGD - Stochastic Gradient Descent (no modifications)";
        if(i_parameters.size() != 2) {
            cout << "ERROR: incorrect number of prams passed to SGD optimizer\n";
            exit(EXIT_FAILURE);
        }

        // [parameters] captures a constant version of parameters for the lambda function
        updateAlg = [i_parameters](Tensor* weights, Tensor* accumalatedGrad, int iterSinceLastUpdate,  
                          Tensor* cache, vector<Tensor*> queue) {
                            
                            double learningRate = i_parameters[0];
                            bool useAvgGrad = i_parameters[1];

                            // we update the weights using the pointers


                            if(useAvgGrad) { // use average grad
                                (*weights) -= learningRate * ((*accumalatedGrad) / iterSinceLastUpdate);
                            } 
                            else { // use cumalative sum grad
                                (*weights) -= learningRate * (*accumalatedGrad);
                            }


                            // the layer's updateWeights() call will handle
                            // zeroing the gradient for us.
                          };

    }

    else {
        cout << "ERROR: input Optimizer name did not match any presets\n";
        exit(EXIT_FAILURE);
    }
}

// **MODEL**

Model::Model(vector<int> shape, vector<Layer*> hiddenLayers, 
                LossFunction* i_lossFunction, Optimizer* i_optimizer) {

    inputLayer = new InputLayer(shape);
    outputLayer = new OutputLayer();
    lossFunction = i_lossFunction;
    optimizer = i_optimizer;

    // allowing no hidden layer models for the purposes of testing.
    if(hiddenLayers.size() == 0) {
        cout << "WARNING: no hidden layer inputted";
        inputLayer->setNextLayer(outputLayer);
        outputLayer->setPrevLayer(inputLayer);
    }
    
    else { // form hidden layers into a linked list
        
        // this logic can prolly be simplified later

        inputLayer->setNextLayer(hiddenLayers[0]);
        hiddenLayers[0]->setPrevLayer(inputLayer);

        for(int i = 0; i < hiddenLayers.size(); i++) {
    
        if(i < hiddenLayers.size()-1) { // if it is not the last hiddenLayer
            hiddenLayers[i]->setNextLayer(hiddenLayers[i+1]);
        }
        if(i > 0) { // if it is not the first hiddenLayer
            hiddenLayers[i]->setPrevLayer(hiddenLayers[i-1]);
        }

        outputLayer->setPrevLayer(hiddenLayers[hiddenLayers.size()-1]);
        hiddenLayers[hiddenLayers.size()-1]->setNextLayer(outputLayer);

        }

    }
    
}

Tensor Model::predict(const Tensor& input) {
     
     inputLayer->feedForward(input);
     return outputLayer->getLastOutput();

}

double Model::train(const Tensor& input, const Tensor& target) {

    Tensor prediction = predict(input);

    // debug cout
    if(0) {
        cout << "\n\nTRAIN: INPUT\n"; 
        input.print();

        cout << "\nTRAIN: PREDICTED\n";
        prediction.print();

        cout << "\nTRAIN: TARGET\n";
        target.print();

    }

    double loss = lossFunction->getLoss(prediction, target);

    if(0) {
        cout << "TRAIN: LOSS = "<< loss <<"\n";
    }

    Tensor J_output = lossFunction->getGradient(prediction, target);
    outputLayer->backPropagate(J_output); // this will have all the 
                                          // layers save their grad
    return loss;
}

void Model::update() {
    outputLayer->updateWeights(optimizer);
}

void Model::print() {

    cout << "\nMODEL DETAILS:\n";
    cout << "\tLayer Sequence:\n";

    Layer* curr = inputLayer;
    while(curr != nullptr) {
        cout << "\t\t" << curr->name << "\n";
        curr = curr->nextLayer;
    }
    cout << "\tLoss Function:\n";
    cout << "\t\t" << lossFunction->name << "\n";
    cout << "\tOptimizer:\n";
    cout << "\t\t" << optimizer->name << "\n";
    cout << "\n";


    // COMPLETE PRINT FUNCTION FOR EACH LAYER THAN COME BACK;
    // cout << "LAYER DETAILS:\n"
    // inputLayer->

}

